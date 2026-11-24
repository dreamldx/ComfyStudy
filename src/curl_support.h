// The parts of the libcurl code that more than one source file needs: the one-time global
// initialization, owners for libcurl's handles, and Transfer (the options and buffers of one request).
// Internal: nothing outside src/ includes this.
#pragma once

#include <curl/curl.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "http_client.h"

namespace http::detail {

// libcurl wants curl_global_init once per process, before the first transfer.
struct CurlGlobal {
  CurlGlobal() {
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) throw Error("curl_global_init failed");
  }
  ~CurlGlobal() { curl_global_cleanup(); }
};

inline void EnsureGlobalInit() { static const CurlGlobal global; }

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
inline std::size_t AppendBody(char* data, std::size_t size, std::size_t count, void* user) {
  static_cast<std::string*>(user)->append(data, size * count);
  return size * count;  // returning less would abort the transfer
}

inline void Set(CURLcode code, const char* option) {
  if (code != CURLE_OK) {
    throw Error(std::string("cannot set curl option ") + option + ": " + curl_easy_strerror(code));
  }
}

inline ListPtr MakeHeaderList(const std::vector<Header>& headers) {
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

inline long VersionOption(Version version) {
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

inline std::chrono::microseconds Micros(CURL* handle, CURLINFO info) {
  curl_off_t value = 0;
  curl_easy_getinfo(handle, info, &value);
  return std::chrono::microseconds(value);
}

inline const char* ProtocolName(long version) {
  switch (version) {
    case CURL_HTTP_VERSION_1_0: return "HTTP/1.0";
    case CURL_HTTP_VERSION_1_1: return "HTTP/1.1";
    case CURL_HTTP_VERSION_2_0: return "HTTP/2";
    case CURL_HTTP_VERSION_3: return "HTTP/3";
    default: return "unknown";
  }
}

// Reads the status and the timings of a finished transfer.
inline Response CollectResponse(CURL* handle, std::string body) {
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

}  // namespace http::detail
