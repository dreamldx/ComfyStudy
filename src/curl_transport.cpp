#include "curl_transport.h"

#include <curl/curl.h>

#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <thread>

namespace http {

namespace asio = boost::asio;

namespace {

// --- libcurl's C interface, wrapped ---------------------------------------------------------------

// libcurl wants curl_global_init once per process, before the first transfer.
struct CurlGlobal {
  CurlGlobal() {
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) throw Error("curl_global_init failed");
  }
  ~CurlGlobal() { curl_global_cleanup(); }
};

void EnsureGlobalInit() { static const CurlGlobal global; }

struct EasyDeleter {
  void operator()(CURL* handle) const { curl_easy_cleanup(handle); }
};
struct ListDeleter {
  void operator()(curl_slist* list) const { curl_slist_free_all(list); }
};
struct MultiDeleter {
  void operator()(CURLM* handle) const { curl_multi_cleanup(handle); }
};

using EasyPtr = std::unique_ptr<CURL, EasyDeleter>;
using ListPtr = std::unique_ptr<curl_slist, ListDeleter>;

// libcurl calls this for every piece of the response body that arrives.
std::size_t AppendBody(char* data, std::size_t size, std::size_t count, void* user) {
  static_cast<std::string*>(user)->append(data, size * count);
  return size * count;  // returning less would abort the transfer
}

void Set(CURLcode code, const char* option) {
  if (code != CURLE_OK) {
    throw Error(std::string("cannot set curl option ") + option + ": " + curl_easy_strerror(code));
  }
}

ListPtr MakeHeaderList(const std::vector<Header>& headers) {
  ListPtr list;
  for (const Header& header : headers) {
    const std::string line = header.first + ": " + header.second;
    curl_slist* extended = curl_slist_append(list.get(), line.c_str());
    if (!extended) throw Error("out of memory while building the request headers");
    list.release();  // curl_slist_append already returned the (possibly new) head
    list.reset(extended);
  }
  return list;
}

// What went wrong, in terms of what the caller may do about it.
ErrorKind KindOf(CURLcode code) {
  switch (code) {
    case CURLE_COULDNT_RESOLVE_PROXY:
    case CURLE_COULDNT_RESOLVE_HOST:
    case CURLE_COULDNT_CONNECT:
    case CURLE_SSL_CONNECT_ERROR:
    case CURLE_PEER_FAILED_VERIFICATION:
      return ErrorKind::kConnect;
    case CURLE_OPERATION_TIMEDOUT:
      return ErrorKind::kTimeout;
    case CURLE_SEND_ERROR:
    case CURLE_RECV_ERROR:
    case CURLE_GOT_NOTHING:
    case CURLE_PARTIAL_FILE:
    case CURLE_HTTP2:
    case CURLE_HTTP2_STREAM:
      return ErrorKind::kLost;
    default:
      return ErrorKind::kOther;
  }
}

// Everything that belongs to one request. `body`, `headers`, `response_body` and `error_text` must
// stay alive until the transfer is over, so they live here and the Transfer must not move.
struct Transfer {
  std::string body;
  ListPtr headers;
  std::string response_body;
  char error_text[CURL_ERROR_SIZE] = {};

  void Configure(CURL* handle, PostRequest& request, const Options& options) {
    body = std::move(request.body);
    headers = MakeHeaderList(request.headers);

    Set(curl_easy_setopt(handle, CURLOPT_URL, request.url.c_str()), "URL");
    Set(curl_easy_setopt(handle, CURLOPT_POST, 1L), "POST");
    Set(curl_easy_setopt(handle, CURLOPT_POSTFIELDS, body.c_str()), "POSTFIELDS");
    Set(curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(body.size())), "POSTFIELDSIZE_LARGE");
    Set(curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers.get()), "HTTPHEADER");
    Set(curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, &AppendBody), "WRITEFUNCTION");
    Set(curl_easy_setopt(handle, CURLOPT_WRITEDATA, &response_body), "WRITEDATA");
    Set(curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT_MS, static_cast<long>(options.connect_timeout.count())), "CONNECTTIMEOUT_MS");
    Set(curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, static_cast<long>(options.timeout.count())), "TIMEOUT_MS");
    Set(curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 0L), "FOLLOWLOCATION");
    Set(curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1L), "NOSIGNAL");
    Set(curl_easy_setopt(handle, CURLOPT_USERAGENT, "comfystudy"), "USERAGENT");
    Set(curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, error_text), "ERRORBUFFER");
    // Over TLS the server says in the handshake whether it speaks HTTP/2. Then it is better to wait
    // for the first connection and send the other requests over it than to open one connection for
    // each. Over plain http libcurl cannot know that before the first answer has arrived, so waiting
    // would only make the requests queue up.
    if (request.url.rfind("https://", 0) == 0) Set(curl_easy_setopt(handle, CURLOPT_PIPEWAIT, 1L), "PIPEWAIT");
  }

  Error Failure(CURLcode code) const {
    return Error(std::string("request failed: ") + (error_text[0] != '\0' ? error_text : curl_easy_strerror(code)), KindOf(code));
  }
};

// --- from a coroutine to a callback and back ------------------------------------------------------

// What a request calls when it is over: with an error, or with the answer. Can be called from any
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
          // post, not dispatch: the coroutine must not resume inside libcurl's thread or callback.
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

// --- the transport: one thread runs the multi interface ---------------------------------------------

class CurlTransport final : public Transport {
 public:
  explicit CurlTransport(Options options) : options_(options) {
    EnsureGlobalInit();
    multi_.reset(curl_multi_init());
    if (!multi_) throw Error("curl_multi_init failed");
    thread_ = std::thread([this] { Run(); });
  }

  ~CurlTransport() override {
    {
      const std::lock_guard<std::mutex> lock(mutex_);
      stopping_ = true;
    }
    curl_multi_wakeup(multi_.get());
    thread_.join();
  }

  asio::awaitable<Response> Post(PostRequest request) override {
    co_return co_await Suspend([this, request = std::move(request)](Done done, asio::cancellation_slot slot) mutable {
      auto job = std::make_unique<Job>();
      job->request = std::move(request);
      job->done = std::move(done);
      std::uint64_t id = 0;
      {
        const std::lock_guard<std::mutex> lock(mutex_);
        id = next_id_++;
        job->id = id;
        incoming_.push_back(std::move(job));
      }
      if (slot.is_connected()) {
        slot.assign([this, id](asio::cancellation_type) {
          {
            const std::lock_guard<std::mutex> lock(mutex_);
            cancels_.push_back(id);
          }
          curl_multi_wakeup(multi_.get());
        });
      }
      curl_multi_wakeup(multi_.get());  // the one call that is meant to be made from other threads
    });
  }

 private:
  struct Job {
    std::uint64_t id = 0;
    PostRequest request;
    Transfer transfer;  // before `easy`: destroyed after the handle, which may still touch its buffers
    EasyPtr easy;
    Done done;
  };
  using Running = std::map<std::uint64_t, std::unique_ptr<Job>>;

  // --- the curl thread only, from here on ---

  void Add(std::unique_ptr<Job> job) {
    try {
      job->easy.reset(curl_easy_init());
      if (!job->easy) throw Error("curl_easy_init failed");
      job->transfer.Configure(job->easy.get(), job->request, options_);
      Set(curl_easy_setopt(job->easy.get(), CURLOPT_PRIVATE, job.get()), "PRIVATE");
      if (curl_multi_add_handle(multi_.get(), job->easy.get()) != CURLM_OK) throw Error("curl_multi_add_handle failed");
    } catch (const Error& e) {
      job->done(std::make_exception_ptr(e), {});
      return;
    }
    const std::uint64_t id = job->id;
    running_.emplace(id, std::move(job));
  }

  // Ends one request: takes it out of libcurl and tells the coroutine.
  void Finish(Running::iterator it, std::exception_ptr error, std::optional<Response> response) {
    std::unique_ptr<Job> job = std::move(it->second);
    running_.erase(it);
    curl_multi_remove_handle(multi_.get(), job->easy.get());
    job->done(std::move(error), response ? std::move(*response) : Response{});
  }

  void FailAll(const std::string& why) {
    while (!running_.empty()) Finish(running_.begin(), std::make_exception_ptr(Error(why)), std::nullopt);
  }

  void Run() {
    for (;;) {
      std::vector<std::unique_ptr<Job>> added;
      std::vector<std::uint64_t> cancelled;
      bool stop = false;
      {
        const std::lock_guard<std::mutex> lock(mutex_);
        added.swap(incoming_);
        cancelled.swap(cancels_);
        stop = stopping_;
      }
      for (auto& job : added) Add(std::move(job));
      for (const std::uint64_t id : cancelled) {
        const auto it = running_.find(id);
        if (it != running_.end()) Finish(it, Cancelled(), std::nullopt);
      }
      if (stop) {
        FailAll("the transport was destroyed while the request was running");
        return;
      }

      int still_running = 0;
      const CURLMcode performed = curl_multi_perform(multi_.get(), &still_running);
      if (performed != CURLM_OK) FailAll(std::string("curl_multi_perform failed: ") + curl_multi_strerror(performed));

      int pending = 0;
      while (CURLMsg* message = curl_multi_info_read(multi_.get(), &pending)) {
        if (message->msg != CURLMSG_DONE) continue;
        Job* job = nullptr;
        curl_easy_getinfo(message->easy_handle, CURLINFO_PRIVATE, &job);
        const auto it = running_.find(job->id);
        if (message->data.result == CURLE_OK) {
          Response response;
          curl_easy_getinfo(job->easy.get(), CURLINFO_RESPONSE_CODE, &response.status);
          response.body = std::move(job->transfer.response_body);
          Finish(it, nullptr, std::move(response));
        } else {
          Finish(it, std::make_exception_ptr(job->transfer.Failure(message->data.result)), std::nullopt);
        }
      }
      // Sleep until a transfer has something to do, or curl_multi_wakeup is called from another thread.
      curl_multi_poll(multi_.get(), nullptr, 0, 1000, nullptr);
    }
  }

  Options options_;
  std::unique_ptr<CURLM, MultiDeleter> multi_;
  std::mutex mutex_;  // protects what the other threads hand to the curl thread:
  std::vector<std::unique_ptr<Job>> incoming_;
  std::vector<std::uint64_t> cancels_;
  bool stopping_ = false;
  std::uint64_t next_id_ = 1;
  Running running_;   // the curl thread only
  std::thread thread_;  // last: started when everything else exists
};

}  // namespace

std::unique_ptr<Transport> MakeCurlTransport(Options options) { return std::make_unique<CurlTransport>(options); }

}  // namespace http
