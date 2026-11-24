#include "http_client.h"

#include <curl/curl.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <mutex>

namespace http {
namespace {

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

long VersionOption(Version version) {
  switch (version) {
    case Version::kHttp1_1: return CURL_HTTP_VERSION_1_1;
    case Version::kHttp2: return CURL_HTTP_VERSION_2TLS;
    case Version::kAny: break;
  }
  return CURL_HTTP_VERSION_NONE;
}

// Everything that belongs to one request. `body`, `headers`, `response_body` and `error_text` must
// stay alive until the transfer is over, so they live here and the Transfer must not move.
struct Transfer {
  std::string body;
  ListPtr headers;
  std::string response_body;
  char error_text[CURL_ERROR_SIZE] = {};

  // Sets all the options of `handle` for this request. (A handle may be configured again and again:
  // its connection stays in its cache.)
  void Configure(CURL* handle, const std::string& url, const std::vector<Header>& header_list,
                 std::string request_body, const Options& options) {
    body = std::move(request_body);
    headers = MakeHeaderList(header_list);
    response_body.clear();
    error_text[0] = '\0';

    Set(curl_easy_setopt(handle, CURLOPT_URL, url.c_str()), "URL");
    Set(curl_easy_setopt(handle, CURLOPT_POST, 1L), "POST");
    Set(curl_easy_setopt(handle, CURLOPT_POSTFIELDS, body.c_str()), "POSTFIELDS");
    Set(curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(body.size())),
        "POSTFIELDSIZE_LARGE");
    Set(curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers.get()), "HTTPHEADER");
    Set(curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, &AppendBody), "WRITEFUNCTION");
    Set(curl_easy_setopt(handle, CURLOPT_WRITEDATA, &response_body), "WRITEDATA");
    Set(curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT_MS,
                         static_cast<long>(options.connect_timeout.count())),
        "CONNECTTIMEOUT_MS");
    Set(curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, static_cast<long>(options.timeout.count())),
        "TIMEOUT_MS");
    Set(curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 0L), "FOLLOWLOCATION");
    Set(curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1L), "NOSIGNAL");
    Set(curl_easy_setopt(handle, CURLOPT_USERAGENT, "comfystudy"), "USERAGENT");
    Set(curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, error_text), "ERRORBUFFER");
    Set(curl_easy_setopt(handle, CURLOPT_HTTP_VERSION, VersionOption(options.version)), "HTTP_VERSION");
    Set(curl_easy_setopt(handle, CURLOPT_SHARE, options.share ? options.share->handle() : nullptr),
        "SHARE");
  }

  std::string Failure(CURLcode code) const {
    return std::string("request failed: ") +
           (error_text[0] != '\0' ? error_text : curl_easy_strerror(code));
  }
};

std::chrono::microseconds Micros(CURL* handle, CURLINFO info) {
  curl_off_t value = 0;
  curl_easy_getinfo(handle, info, &value);
  return std::chrono::microseconds(value);
}

const char* ProtocolName(long version) {
  switch (version) {
    case CURL_HTTP_VERSION_1_0: return "HTTP/1.0";
    case CURL_HTTP_VERSION_1_1: return "HTTP/1.1";
    case CURL_HTTP_VERSION_2_0: return "HTTP/2";
    case CURL_HTTP_VERSION_3: return "HTTP/3";
    default: return "unknown";
  }
}

// Reads the status and the timings of a finished transfer.
Response CollectResponse(CURL* handle, std::string body) {
  Response response;
  response.body = std::move(body);
  Set(curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &response.status), "RESPONSE_CODE");

  Timing& timing = response.timing;
  timing.name_lookup = Micros(handle, CURLINFO_NAMELOOKUP_TIME_T);
  timing.connect = Micros(handle, CURLINFO_CONNECT_TIME_T);
  timing.tls = Micros(handle, CURLINFO_APPCONNECT_TIME_T);
  timing.first_byte = Micros(handle, CURLINFO_STARTTRANSFER_TIME_T);
  timing.total = Micros(handle, CURLINFO_TOTAL_TIME_T);
  curl_easy_getinfo(handle, CURLINFO_NUM_CONNECTS, &timing.new_connections);
  long version = 0;
  curl_easy_getinfo(handle, CURLINFO_HTTP_VERSION, &version);
  timing.protocol = ProtocolName(version);
  return response;
}

}  // namespace

// --- Session and Post ----------------------------------------------------------------------------

struct Session::Impl {
  // The Transfer comes first, so it is destroyed AFTER the handle: while a handle is cleaned up,
  // libcurl may still touch the buffers that its options point to.
  Transfer transfer;
  EasyPtr easy;
  Options options;
};

Session::Session(Options options) : impl_(std::make_unique<Impl>()) {
  EnsureGlobalInit();
  impl_->easy.reset(curl_easy_init());
  if (!impl_->easy) throw Error("curl_easy_init failed");
  impl_->options = options;
}

Session::~Session() = default;

Response Session::Post(const std::string& url, const std::vector<Header>& headers,
                       const std::string& body) {
  Transfer& transfer = impl_->transfer;
  CURL* handle = impl_->easy.get();
  transfer.Configure(handle, url, headers, body, impl_->options);
  const CURLcode code = curl_easy_perform(handle);
  if (code != CURLE_OK) throw Error(transfer.Failure(code));
  return CollectResponse(handle, std::move(transfer.response_body));
}

Response Post(const std::string& url, const std::vector<Header>& headers, const std::string& body,
              const Options& options) {
  Session session(options);  // a new handle: nothing is kept for the next call
  return session.Post(url, headers, body);
}

// --- Share ---------------------------------------------------------------------------------------

using ShareLocks = std::array<std::recursive_mutex, CURL_LOCK_DATA_LAST>;  // one lock per kind of shared data

struct Share::Impl {
  // The locks come first, so they are destroyed AFTER the share handle: curl_share_cleanup still
  // calls the lock callbacks.
  ShareLocks locks;
  std::unique_ptr<CURLSH, void (*)(CURLSH*)> handle{nullptr, [](CURLSH* h) { curl_share_cleanup(h); }};
};

namespace {
void LockShare(CURL*, curl_lock_data data, curl_lock_access, void* user) {
  static_cast<ShareLocks*>(user)->at(static_cast<std::size_t>(data)).lock();
}
void UnlockShare(CURL*, curl_lock_data data, void* user) {
  static_cast<ShareLocks*>(user)->at(static_cast<std::size_t>(data)).unlock();
}
void ShareSet(CURLSHcode code, const char* option) {
  if (code != CURLSHE_OK) {
    throw Error(std::string("cannot set curl share option ") + option + ": " + curl_share_strerror(code));
  }
}
}  // namespace

Share::Share() : impl_(std::make_unique<Impl>()) {
  EnsureGlobalInit();
  impl_->handle.reset(curl_share_init());
  if (!impl_->handle) throw Error("curl_share_init failed");
  CURLSH* share = impl_->handle.get();
  ShareSet(curl_share_setopt(share, CURLSHOPT_LOCKFUNC, &LockShare), "LOCKFUNC");
  ShareSet(curl_share_setopt(share, CURLSHOPT_UNLOCKFUNC, &UnlockShare), "UNLOCKFUNC");
  ShareSet(curl_share_setopt(share, CURLSHOPT_USERDATA, &impl_->locks), "USERDATA");
  ShareSet(curl_share_setopt(share, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS), "SHARE DNS");
  ShareSet(curl_share_setopt(share, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION), "SHARE SSL_SESSION");
  // CURL_LOCK_DATA_CONNECT (the connection cache) is left out on purpose. With it, several threads
  // crashed inside libcurl 8.23 (url_match_destination) in about 1 of 6 runs of the Debug build and
  // 1 of 60 of the Release build; without it, no run in 60 crashed. A thread that wants to reuse
  // connections keeps its own Session instead.
}

Share::~Share() = default;

void* Share::handle() const { return impl_->handle.get(); }

// --- PostMany ------------------------------------------------------------------------------------

namespace {

// One request under way inside PostMany.
struct Job {
  std::size_t index = 0;
  Transfer transfer;  // before `easy`: destroyed after the handle (see Session::Impl)
  EasyPtr easy;
};

}  // namespace

std::vector<PostResult> PostMany(const std::vector<PostRequest>& requests, std::size_t max_in_flight,
                                 const Options& options) {
  EnsureGlobalInit();
  std::vector<PostResult> results(requests.size());
  if (requests.empty()) return results;
  if (max_in_flight == 0) max_in_flight = 1;

  std::unique_ptr<CURLM, MultiDeleter> multi(curl_multi_init());
  if (!multi) throw Error("curl_multi_init failed");

  std::vector<std::unique_ptr<Job>> running;  // the jobs that are added to `multi` right now
  std::size_t next = 0;                       // the next request to start

  auto start_more = [&] {
    while (next < requests.size() && running.size() < max_in_flight) {
      auto job = std::make_unique<Job>();
      job->index = next;
      PostResult& result = results[next];
      ++next;
      job->easy.reset(curl_easy_init());
      if (!job->easy) {
        result.error = "curl_easy_init failed";
        continue;
      }
      try {
        const PostRequest& request = requests[job->index];
        job->transfer.Configure(job->easy.get(), request.url, request.headers, request.body, options);
        // Over TLS the server says in the handshake whether it speaks HTTP/2. Then it is better to
        // wait for the first connection and send the other requests over it than to open one
        // connection for each. Over plain http libcurl cannot know that before the first answer
        // has arrived, so waiting would only serialize the requests: 8 requests that take 200 ms
        // each needed 421 ms instead of 218 ms.
        const bool tls = request.url.rfind("https://", 0) == 0;
        if (tls && options.version != Version::kHttp1_1) {
          Set(curl_easy_setopt(job->easy.get(), CURLOPT_PIPEWAIT, 1L), "PIPEWAIT");
        }
        Set(curl_easy_setopt(job->easy.get(), CURLOPT_PRIVATE, job.get()), "PRIVATE");
      } catch (const Error& e) {
        result.error = e.what();
        continue;
      }
      if (curl_multi_add_handle(multi.get(), job->easy.get()) != CURLM_OK) {
        result.error = "curl_multi_add_handle failed";
        continue;
      }
      running.push_back(std::move(job));
    }
  };

  start_more();
  while (!running.empty()) {
    int still_running = 0;
    const CURLMcode performed = curl_multi_perform(multi.get(), &still_running);
    if (performed != CURLM_OK) {
      for (auto& job : running) {
        results[job->index].error = std::string("curl_multi_perform failed: ") + curl_multi_strerror(performed);
        curl_multi_remove_handle(multi.get(), job->easy.get());
      }
      running.clear();
      break;
    }

    // Collect the transfers that are done, and start the next ones in their place.
    int pending = 0;
    while (CURLMsg* message = curl_multi_info_read(multi.get(), &pending)) {
      if (message->msg != CURLMSG_DONE) continue;
      Job* job = nullptr;
      curl_easy_getinfo(message->easy_handle, CURLINFO_PRIVATE, &job);
      PostResult& result = results[job->index];
      if (message->data.result == CURLE_OK) {
        result.response = CollectResponse(job->easy.get(), std::move(job->transfer.response_body));
      } else {
        result.error = job->transfer.Failure(message->data.result);
      }
      curl_multi_remove_handle(multi.get(), job->easy.get());
      for (auto it = running.begin(); it != running.end(); ++it) {
        if (it->get() == job) {
          running.erase(it);
          break;
        }
      }
    }
    start_more();

    if (!running.empty()) {
      // Sleep until some transfer has something to do (or 1 second has passed).
      curl_multi_poll(multi.get(), nullptr, 0, 1000, nullptr);
    }
  }
  return results;
}

// --- Redacted ------------------------------------------------------------------------------------

namespace {

std::string Lower(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return text;
}

}  // namespace

std::vector<Header> Redacted(const std::vector<Header>& headers) {
  std::vector<Header> result;
  result.reserve(headers.size());
  for (const Header& header : headers) {
    const std::string name = Lower(header.first);
    if (name == "authorization") {
      // Keep the scheme ("Bearer", "Basic"): it says what kind of credential it was.
      const std::size_t space = header.second.find(' ');
      result.push_back({header.first, space == std::string::npos ? "***" : header.second.substr(0, space) + " ***"});
    } else if (name == "x-api-key" || name == "api-key" || name == "proxy-authorization" || name == "cookie") {
      result.push_back({header.first, "***"});
    } else {
      result.push_back(header);
    }
  }
  return result;
}

}  // namespace http
