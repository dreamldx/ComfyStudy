#include "async_http.h"

#include <curl/curl.h>

#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <thread>

#include "curl_support.h"
#include "event_loop.h"

namespace http {

using namespace detail;
namespace asio = boost::asio;

namespace {

// What a request calls when it is over: with an exception, or with the answer. Can be called from any
// thread, once.
using Done = std::function<void(std::exception_ptr, Response)>;

// Turns "start the request, then call `done`" into something a coroutine can await. `start` receives
// `done` and the cancellation slot of the awaiting coroutine (connected when it can be cancelled).
// `done` resumes the coroutine through its own executor, so it is safe to call from another thread.
template <class Start>
asio::awaitable<Response> Suspend(Start start) {
  co_return co_await asio::async_initiate<decltype(asio::use_awaitable), void(std::exception_ptr, Response)>(
      [start = std::move(start)](auto handler) mutable {
        using Handler = decltype(handler);
        asio::any_io_executor executor = asio::get_associated_executor(handler);
        asio::cancellation_slot slot = asio::get_associated_cancellation_slot(handler);
        auto owner = std::make_shared<Handler>(std::move(handler));
        // While the request is under way the event loop must not think it has nothing to do.
        auto guard = std::make_shared<asio::executor_work_guard<asio::any_io_executor>>(executor);
        Done done = [owner, guard, executor](std::exception_ptr error, Response response) {
          asio::post(executor, [owner, guard, error = std::move(error), response = std::move(response)]() mutable {
            guard->reset();
            Handler resume = std::move(*owner);
            std::move(resume)(error, std::move(response));
          });
        };
        start(std::move(done), slot);
      },
      asio::use_awaitable);
}

std::exception_ptr Cancelled() {
  return std::make_exception_ptr(boost::system::system_error(asio::error::operation_aborted));
}

// Over TLS the server says in the handshake whether it speaks HTTP/2. Then it is better to wait for
// the first connection and send the other requests over it than to open one connection for each.
// Over plain http libcurl cannot know that before the first answer has arrived, so waiting would only
// serialize the requests.
void WaitForMultiplexing(CURL* easy, const std::string& url, const Options& options) {
  const bool tls = url.rfind("https://", 0) == 0;
  if (tls && options.version != Version::kHttp1_1) Set(curl_easy_setopt(easy, CURLOPT_PIPEWAIT, 1L), "PIPEWAIT");
}

}  // namespace

// --- PoolClient: a pool of threads, a Session each ----------------------------------------------------

struct PoolClient::Impl {
  Impl(std::size_t threads, Options o) : options(std::move(o)), pool(threads) {
    for (std::size_t i = 0; i < threads; ++i) idle.push_back(std::make_unique<Session>(options));
  }

  // One Session for the duration of one request. There are as many Sessions as threads, so a running
  // request always finds one. The one used last is used first: its connection is the warmest.
  class Lease {
   public:
    explicit Lease(Impl& impl) : impl_(impl) {
      const std::lock_guard<std::mutex> lock(impl_.mutex);
      session_ = std::move(impl_.idle.back());
      impl_.idle.pop_back();
    }
    ~Lease() {
      const std::lock_guard<std::mutex> lock(impl_.mutex);
      impl_.idle.push_back(std::move(session_));
    }
    Lease(const Lease&) = delete;
    Lease& operator=(const Lease&) = delete;
    Session& session() { return *session_; }

   private:
    Impl& impl_;
    std::unique_ptr<Session> session_;
  };

  Options options;
  std::mutex mutex;
  std::vector<std::unique_ptr<Session>> idle;
  asio::thread_pool pool;  // last: it is destroyed (and joined) first, while the Sessions still exist
};

PoolClient::PoolClient(std::size_t threads, Options options)
    : impl_(std::make_unique<Impl>(threads == 0 ? 1 : threads, std::move(options))) {}

PoolClient::~PoolClient() = default;

asio::awaitable<Response> PoolClient::Post(PostRequest request) {
  Impl* impl = impl_.get();
  co_return co_await comfystudy::loop::OnPool(impl->pool, [impl, request = std::move(request)]() -> Response {
    Impl::Lease lease(*impl);
    return lease.session().Post(request.url, request.headers, request.body);
  });
}

// --- CurlThreadClient: one thread runs the multi interface -------------------------------------------------

struct CurlThreadClient::Impl {
  struct Job {
    std::uint64_t id = 0;
    PostRequest request;
    Transfer transfer;  // before `easy`: destroyed after the handle (see Session::Impl)
    EasyPtr easy;
    Done done;
  };

  explicit Impl(Options o) : options(std::move(o)) {
    EnsureGlobalInit();
    multi.reset(curl_multi_init());
    if (!multi) throw Error("curl_multi_init failed");
    thread = std::thread([this] { Run(); });
  }

  ~Impl() {
    {
      const std::lock_guard<std::mutex> lock(mutex);
      stopping = true;
    }
    curl_multi_wakeup(multi.get());
    thread.join();
  }

  void Wake() { curl_multi_wakeup(multi.get()); }

  // --- called on the curl thread only ---

  void Add(std::unique_ptr<Job> job) {
    try {
      job->easy.reset(curl_easy_init());
      if (!job->easy) throw Error("curl_easy_init failed");
      job->transfer.Configure(job->easy.get(), job->request.url, job->request.headers, std::move(job->request.body), options);
      WaitForMultiplexing(job->easy.get(), job->request.url, options);
      Set(curl_easy_setopt(job->easy.get(), CURLOPT_PRIVATE, job.get()), "PRIVATE");
      if (curl_multi_add_handle(multi.get(), job->easy.get()) != CURLM_OK) throw Error("curl_multi_add_handle failed");
    } catch (const Error&) {
      job->done(std::current_exception(), {});
      return;
    }
    const std::uint64_t id = job->id;
    running.emplace(id, std::move(job));
  }

  void Finish(std::map<std::uint64_t, std::unique_ptr<Job>>::iterator it, std::exception_ptr error, std::optional<Response> response) {
    std::unique_ptr<Job> job = std::move(it->second);
    running.erase(it);
    curl_multi_remove_handle(multi.get(), job->easy.get());
    job->done(std::move(error), response ? std::move(*response) : Response{});
  }

  void Run() {
    for (;;) {
      std::vector<std::unique_ptr<Job>> added;
      std::vector<std::uint64_t> cancelled;
      bool stop = false;
      {
        const std::lock_guard<std::mutex> lock(mutex);
        added.swap(incoming);
        cancelled.swap(cancels);
        stop = stopping;
      }
      for (auto& job : added) Add(std::move(job));
      for (const std::uint64_t id : cancelled) {
        const auto it = running.find(id);
        if (it != running.end()) Finish(it, Cancelled(), std::nullopt);
      }
      if (stop) {
        while (!running.empty()) {
          try {
            throw Error("the client was destroyed while the request was running");
          } catch (const Error&) {
            Finish(running.begin(), std::current_exception(), std::nullopt);
          }
        }
        return;
      }

      int still_running = 0;
      const CURLMcode performed = curl_multi_perform(multi.get(), &still_running);
      if (performed != CURLM_OK) {
        while (!running.empty()) {
          try {
            throw Error(std::string("curl_multi_perform failed: ") + curl_multi_strerror(performed));
          } catch (const Error&) {
            Finish(running.begin(), std::current_exception(), std::nullopt);
          }
        }
      }
      int pending = 0;
      while (CURLMsg* message = curl_multi_info_read(multi.get(), &pending)) {
        if (message->msg != CURLMSG_DONE) continue;
        Job* job = nullptr;
        curl_easy_getinfo(message->easy_handle, CURLINFO_PRIVATE, &job);
        const auto it = running.find(job->id);
        if (message->data.result == CURLE_OK) {
          Response response = CollectResponse(job->easy.get(), std::move(job->transfer.response_body));
          Finish(it, nullptr, std::move(response));
        } else {
          try {
            throw Error(job->transfer.Failure(message->data.result));
          } catch (const Error&) {
            Finish(it, std::current_exception(), std::nullopt);
          }
        }
      }
      // Sleep until a transfer has something to do, or Wake() is called from another thread.
      curl_multi_poll(multi.get(), nullptr, 0, 1000, nullptr);
    }
  }

  Options options;
  std::unique_ptr<CURLM, MultiDeleter> multi;
  std::mutex mutex;  // protects what the other threads hand to the curl thread:
  std::vector<std::unique_ptr<Job>> incoming;
  std::vector<std::uint64_t> cancels;
  bool stopping = false;
  std::uint64_t next_id = 1;
  std::map<std::uint64_t, std::unique_ptr<Job>> running;  // the curl thread only
  std::thread thread;                                     // last: started when everything else exists
};

CurlThreadClient::CurlThreadClient(Options options) : impl_(std::make_unique<Impl>(std::move(options))) {}

CurlThreadClient::~CurlThreadClient() = default;

asio::awaitable<Response> CurlThreadClient::Post(PostRequest request) {
  Impl* impl = impl_.get();
  co_return co_await Suspend([impl, request = std::move(request)](Done done, asio::cancellation_slot slot) mutable {
    auto job = std::make_unique<Impl::Job>();
    job->request = std::move(request);
    job->done = std::move(done);
    std::uint64_t id = 0;
    {
      const std::lock_guard<std::mutex> lock(impl->mutex);
      id = impl->next_id++;
      job->id = id;
      impl->incoming.push_back(std::move(job));
    }
    if (slot.is_connected()) {
      slot.assign([impl, id](asio::cancellation_type) {
        {
          const std::lock_guard<std::mutex> lock(impl->mutex);
          impl->cancels.push_back(id);
        }
        impl->Wake();
      });
    }
    impl->Wake();
  });
}

// --- SocketClient: the event loop watches libcurl's sockets ---------------------------------------------

struct SocketClient::Impl {
  struct Job {
    Transfer transfer;  // before `easy`: destroyed after the handle
    EasyPtr easy;
    Done done;
  };

  // A socket libcurl opened. libcurl owns it (it closes it); the Asio object is only a way to wait
  // for it, so release() it before it is destroyed.
  struct Sock {
    explicit Sock(asio::any_io_executor ex) : socket(ex) {}
    asio::ip::tcp::socket socket;
    bool want_read = false;
    bool want_write = false;
    bool waiting_read = false;
    bool waiting_write = false;
    bool waiting_error = false;
  };

  Impl(asio::any_io_executor ex, Options o) : executor(ex), options(std::move(o)), timer(ex) {
    EnsureGlobalInit();
    multi.reset(curl_multi_init());
    if (!multi) throw Error("curl_multi_init failed");
    curl_multi_setopt(multi.get(), CURLMOPT_SOCKETFUNCTION, &OnSocket);
    curl_multi_setopt(multi.get(), CURLMOPT_SOCKETDATA, this);
    curl_multi_setopt(multi.get(), CURLMOPT_TIMERFUNCTION, &OnTimer);
    curl_multi_setopt(multi.get(), CURLMOPT_TIMERDATA, this);
  }

  ~Impl() {
    timer.cancel();
    while (!jobs.empty()) {
      try {
        throw Error("the client was destroyed while the request was running");
      } catch (const Error&) {
        Finish(jobs.begin(), std::current_exception(), std::nullopt);
      }
    }
    multi.reset();  // closes the connections; libcurl reports each socket as removed on the way
  }

  // libcurl: "watch this socket for reading and/or writing" (what), or "stop watching" (REMOVE).
  static int OnSocket(CURL*, curl_socket_t s, int what, void* user, void*) {
    auto* self = static_cast<Impl*>(user);
    if (what == CURL_POLL_REMOVE) {
      const auto it = self->socks.find(s);
      if (it != self->socks.end()) {
        boost::system::error_code ignored;
        it->second->socket.release(ignored);  // cancels the waits; libcurl closes the socket itself
        self->socks.erase(it);
      }
      return 0;
    }
    auto& sock = self->socks[s];
    if (!sock) {
      sock = std::make_shared<Sock>(self->executor);
      boost::system::error_code ec;
      // The protocol is only used by operations we never call (the socket is only waited for).
      sock->socket.assign(asio::ip::tcp::v4(), s, ec);
      if (ec) {
        self->socks.erase(s);
        return -1;
      }
    }
    sock->want_read = (what & CURL_POLL_IN) != 0;
    sock->want_write = (what & CURL_POLL_OUT) != 0;
    self->Watch(s, sock);
    return 0;
  }

  // libcurl: "wake me in timeout_ms" (-1: no need to).
  static int OnTimer(CURLM*, long timeout_ms, void* user) {
    auto* self = static_cast<Impl*>(user);
    if (timeout_ms < 0) {
      self->timer.cancel();
    } else if (timeout_ms == 0) {
      asio::post(self->executor, [self] { self->Act(CURL_SOCKET_TIMEOUT, 0); });
    } else {
      self->timer.expires_after(std::chrono::milliseconds(timeout_ms));
      self->timer.async_wait([self](const boost::system::error_code& ec) {
        if (!ec) self->Act(CURL_SOCKET_TIMEOUT, 0);
      });
    }
    return 0;
  }

  // Starts the waits libcurl asked for on one socket (a wait that is already running stays).
  void Watch(curl_socket_t s, const std::shared_ptr<Sock>& sock) {
    if (sock->want_read && !sock->waiting_read) {
      sock->waiting_read = true;
      sock->socket.async_wait(asio::ip::tcp::socket::wait_read, [this, s, sock](const boost::system::error_code& ec) {
        sock->waiting_read = false;
        if (ec || !sock->want_read) return;  // cancelled because the socket was removed
        Act(s, CURL_CSELECT_IN);
        const auto it = socks.find(s);
        if (it != socks.end() && it->second == sock) Watch(s, sock);
      });
    }
    if (sock->want_write && !sock->waiting_write) {
      sock->waiting_write = true;
      sock->socket.async_wait(asio::ip::tcp::socket::wait_write, [this, s, sock](const boost::system::error_code& ec) {
        sock->waiting_write = false;
        if (ec || !sock->want_write) return;
        Act(s, CURL_CSELECT_OUT);
        const auto it = socks.find(s);
        if (it != socks.end() && it->second == sock) Watch(s, sock);
      });
    }
    // On Windows a connection that fails (refused, unreachable) is NOT reported as "writable": it
    // shows up in the exception set of select(). Without this wait the failure goes unnoticed until
    // libcurl's own connect timeout runs out.
    if ((sock->want_read || sock->want_write) && !sock->waiting_error) {
      sock->waiting_error = true;
      sock->socket.async_wait(asio::ip::tcp::socket::wait_error, [this, s, sock](const boost::system::error_code& ec) {
        sock->waiting_error = false;
        if (ec || !(sock->want_read || sock->want_write)) return;
        Act(s, CURL_CSELECT_ERR);
        const auto it = socks.find(s);
        if (it != socks.end() && it->second == sock) Watch(s, sock);
      });
    }
  }

  // Tells libcurl that something happened (a socket is ready, or the time it asked for has come), then
  // collects the transfers that are done.
  void Act(curl_socket_t s, int events) {
    int still_running = 0;
    curl_multi_socket_action(multi.get(), s, events, &still_running);
    int pending = 0;
    while (CURLMsg* message = curl_multi_info_read(multi.get(), &pending)) {
      if (message->msg != CURLMSG_DONE) continue;
      Job* job = nullptr;
      curl_easy_getinfo(message->easy_handle, CURLINFO_PRIVATE, &job);
      const auto it = jobs.find(job->easy.get());
      if (message->data.result == CURLE_OK) {
        Response response = CollectResponse(job->easy.get(), std::move(job->transfer.response_body));
        Finish(it, nullptr, std::move(response));
      } else {
        try {
          throw Error(job->transfer.Failure(message->data.result));
        } catch (const Error&) {
          Finish(it, std::current_exception(), std::nullopt);
        }
      }
    }
  }

  void Finish(std::map<CURL*, std::unique_ptr<Job>>::iterator it, std::exception_ptr error, std::optional<Response> response) {
    std::unique_ptr<Job> job = std::move(it->second);
    jobs.erase(it);
    curl_multi_remove_handle(multi.get(), job->easy.get());
    job->done(std::move(error), response ? std::move(*response) : Response{});
  }

  void Start(PostRequest request, Done done, asio::cancellation_slot slot) {
    auto job = std::make_unique<Job>();
    job->done = std::move(done);
    try {
      job->easy.reset(curl_easy_init());
      if (!job->easy) throw Error("curl_easy_init failed");
      job->transfer.Configure(job->easy.get(), request.url, request.headers, std::move(request.body), options);
      WaitForMultiplexing(job->easy.get(), request.url, options);
      Set(curl_easy_setopt(job->easy.get(), CURLOPT_PRIVATE, job.get()), "PRIVATE");
    } catch (const Error&) {
      job->done(std::current_exception(), {});
      return;
    }
    CURL* key = job->easy.get();
    jobs.emplace(key, std::move(job));
    if (curl_multi_add_handle(multi.get(), key) != CURLM_OK) {
      try {
        throw Error("curl_multi_add_handle failed");
      } catch (const Error&) {
        Finish(jobs.find(key), std::current_exception(), std::nullopt);
      }
      return;
    }
    if (slot.is_connected()) {
      slot.assign([this, key](asio::cancellation_type) {
        const auto it = jobs.find(key);
        if (it != jobs.end()) Finish(it, Cancelled(), std::nullopt);
      });
    }
  }

  asio::any_io_executor executor;
  Options options;
  asio::steady_timer timer;
  std::unique_ptr<CURLM, MultiDeleter> multi;
  std::map<curl_socket_t, std::shared_ptr<Sock>> socks;
  std::map<CURL*, std::unique_ptr<Job>> jobs;
};

SocketClient::SocketClient(asio::any_io_executor executor, Options options)
    : impl_(std::make_unique<Impl>(std::move(executor), std::move(options))) {}

SocketClient::~SocketClient() = default;

asio::awaitable<Response> SocketClient::Post(PostRequest request) {
  Impl* impl = impl_.get();
  co_return co_await Suspend([impl, request = std::move(request)](Done done, asio::cancellation_slot slot) mutable {
    impl->Start(std::move(request), std::move(done), slot);
  });
}

}  // namespace http
