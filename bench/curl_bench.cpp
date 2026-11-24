// Measures the ways to send many requests with libcurl:
//   curl_bench local [N] [DELAY_MS] [THREADS] [REPS]
//       N requests to a small server on this machine that answers each one after DELAY_MS
//       milliseconds (a stand-in for a model that takes a while to answer); plain HTTP/1.1.
//   curl_bench llm [N] [THREADS] [VERSION] [config options...]
//       N short questions to the service that the settings (config file, environment) name.
//       VERSION is any, 1.1 or 2.
//   curl_bench probe
//       What the three services say about HTTP/2: the protocol that was negotiated, the limit on
//       concurrent streams, the Alt-Svc header. Needs no key (the requests are refused with 401).
//   curl_bench idle
//       Does a connection survive 5, 30 and 90 seconds of silence? One thread per service.
//   curl_bench stream [config options...]
//       One streaming question (Server-Sent Events) to the configured service, over HTTP/2.
//   curl_bench mux [N1,N2,...] [config options...]
//       Does HTTP/2 multiplexing work, and how far? For each N (default 12,50,100) sends N questions
//       at the same time through one CurlThreadClient, over HTTP/1.1 and over HTTP/2.
// The strategies named "async" are the three coroutine clients of src/async_http.h.
#include <algorithm>
#include <boost/asio.hpp>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <mutex>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <curl/curl.h>
#include "json.h"

#include "async_http.h"
#include "config.h"
#include "http_client.h"
#include "llm_wire.h"
#include "options.h"
#include "test_server.h"

namespace {

using Clock = std::chrono::steady_clock;
using Results = std::vector<std::optional<http::Response>>;

// --- the ways to send the requests ---------------------------------------------------------------

Results SequentialNewHandle(const std::vector<http::PostRequest>& requests, const http::Options& options) {
  Results results(requests.size());
  for (std::size_t i = 0; i < requests.size(); ++i) {
    try {
      results[i] = http::Post(requests[i].url, requests[i].headers, requests[i].body, options);
    } catch (const http::Error&) {
    }
  }
  return results;
}

Results SequentialSession(const std::vector<http::PostRequest>& requests, const http::Options& options) {
  Results results(requests.size());
  http::Session session(options);
  for (std::size_t i = 0; i < requests.size(); ++i) {
    try {
      results[i] = session.Post(requests[i].url, requests[i].headers, requests[i].body);
    } catch (const http::Error&) {
    }
  }
  return results;
}

enum class PoolMode { kNewHandle, kShare, kSessionPerThread };

// `threads` threads take the requests one after the other; each request is sent with libcurl's easy
// interface.
Results Pool(const std::vector<http::PostRequest>& requests, std::size_t threads, PoolMode mode,
             http::Options options) {
  Results results(requests.size());
  std::unique_ptr<http::Share> share;
  if (mode == PoolMode::kShare) {
    share = std::make_unique<http::Share>();
    options.share = share.get();
  }
  std::atomic<std::size_t> next{0};
  auto work = [&] {
    std::optional<http::Session> session;
    if (mode == PoolMode::kSessionPerThread) session.emplace(options);
    for (std::size_t i = next++; i < requests.size(); i = next++) {
      try {
        const http::PostRequest& r = requests[i];
        results[i] = session ? session->Post(r.url, r.headers, r.body) : http::Post(r.url, r.headers, r.body, options);
      } catch (const http::Error&) {
      }
    }
  };
  std::vector<std::thread> pool;
  for (std::size_t t = 0; t < threads; ++t) pool.emplace_back(work);
  for (std::thread& t : pool) t.join();
  return results;
}

Results Multi(const std::vector<http::PostRequest>& requests, std::size_t in_flight, const http::Options& options) {
  Results results(requests.size());
  std::vector<http::PostResult> done = http::PostMany(requests, in_flight, options);
  for (std::size_t i = 0; i < done.size(); ++i) results[i] = std::move(done[i].response);
  return results;
}

// --- the measurements ----------------------------------------------------------------------------

struct Row {
  std::string name;
  double wall_ms = 0;
  double average_ms = 0;  // the average time of one request, as libcurl measured it
  long connections = 0;   // new connections made
  std::size_t threads = 1;
  std::size_t ok = 0;
  std::size_t total = 0;
  std::string protocol;
  Results results;
};

template <typename Run>
Row Measure(const std::string& name, std::size_t threads, Run run) {
  Row row;
  row.name = name;
  row.threads = threads;
  const auto start = Clock::now();
  row.results = run();
  row.wall_ms = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
  row.total = row.results.size();
  double sum = 0;
  for (const auto& r : row.results) {
    if (!r) continue;
    ++row.ok;
    row.connections += r->timing.new_connections;
    sum += std::chrono::duration<double, std::milli>(r->timing.total).count();
    row.protocol = r->timing.protocol;
  }
  row.average_ms = row.ok ? sum / static_cast<double>(row.ok) : 0;
  return row;
}

void PrintHeader() {
  std::printf("%-34s %10s %12s %12s %8s %6s  %s\n", "strategy", "total ms", "avg req ms", "new conns", "threads", "ok",
              "protocol");
}

void PrintRow(const Row& r) {
  std::printf("%-34s %10.0f %12.0f %12ld %8zu %3zu/%-3zu %s\n", r.name.c_str(), r.wall_ms, r.average_ms, r.connections,
              r.threads, r.ok, r.total, r.protocol.c_str());
}

// The phases of the first requests of a run, to see what a reused connection saves.
void PrintPhases(const Row& r, std::size_t how_many) {
  std::printf("  phases of the first requests (ms since the start of each request): %s\n", r.name.c_str());
  std::printf("    %-3s %8s %8s %8s %10s %8s %6s\n", "#", "dns", "connect", "tls", "first byte", "total", "conns");
  for (std::size_t i = 0; i < (std::min)(how_many, r.results.size()); ++i) {
    if (!r.results[i]) continue;
    const http::Timing& t = r.results[i]->timing;
    auto ms = [](std::chrono::microseconds us) { return std::chrono::duration<double, std::milli>(us).count(); };
    std::printf("    %-3zu %8.1f %8.1f %8.1f %10.1f %8.1f %6ld\n", i, ms(t.name_lookup), ms(t.connect), ms(t.tls),
                ms(t.first_byte), ms(t.total), t.new_connections);
  }
}

// Runs every strategy `reps` times and keeps the run with the median total time.
// Sends every request at the same time from coroutines on one io_context and waits for all the answers.
// (The clients with a pool of threads still run only as many as they have threads.)
template <typename Client>
Results Async(Client& client, boost::asio::io_context& io, const std::vector<http::PostRequest>& requests) {
  Results results(requests.size());
  auto job = [&](std::size_t i) -> boost::asio::awaitable<void> {
    try {
      results[i] = co_await client.Post(requests[i]);
    } catch (const std::exception&) {
    }
  };
  for (std::size_t i = 0; i < requests.size(); ++i) boost::asio::co_spawn(io, job(i), boost::asio::detached);
  io.restart();
  io.run();
  return results;
}

std::vector<Row> RunAll(const std::vector<http::PostRequest>& requests, std::size_t threads,
                        const http::Options& options, int reps) {
  struct Strategy {
    std::string name;
    std::size_t threads;
    std::function<Results()> run;
  };
  const std::vector<Strategy> strategies = {
      {"sequential, new handle each", 1, [&] { return SequentialNewHandle(requests, options); }},
      {"sequential, one Session", 1, [&] { return SequentialSession(requests, options); }},
      {"pool, new handle each", threads, [&] { return Pool(requests, threads, PoolMode::kNewHandle, options); }},
      {"pool, shared caches (Share)", threads, [&] { return Pool(requests, threads, PoolMode::kShare, options); }},
      {"pool, a Session per thread", threads, [&] { return Pool(requests, threads, PoolMode::kSessionPerThread, options); }},
      {"multi, all in flight", 1, [&] { return Multi(requests, requests.size(), options); }},
      {"multi, " + std::to_string(threads) + " in flight", 1, [&] { return Multi(requests, threads, options); }},
      // The thread counts include the thread that runs the event loop.
      {"async, thread pool + Session", threads + 1, [&] {
         boost::asio::io_context io;
         http::PoolClient client(threads, options);
         return Async(client, io, requests);
       }},
      {"async, one curl thread", 2, [&] {
         boost::asio::io_context io;
         http::CurlThreadClient client(options);
         return Async(client, io, requests);
       }},
      {"async, socket_action", 1, [&] {
         boost::asio::io_context io;
         http::SocketClient client(io.get_executor(), options);
         return Async(client, io, requests);
       }},
  };
  std::vector<Row> rows;
  const auto only = config::Get("BENCH_ONLY");  // run just some strategies, by number: "4,5,9"
  auto wanted = [&](std::size_t index) {
    if (!only) return true;
    for (std::size_t from = 0; from <= only->size();) {
      const std::size_t comma = only->find(',', from);
      const std::string item = only->substr(from, comma == std::string::npos ? std::string::npos : comma - from);
      if (!item.empty() && std::stoul(item) == index) return true;
      if (comma == std::string::npos) break;
      from = comma + 1;
    }
    return false;
  };
  for (std::size_t index = 0; index < strategies.size(); ++index) {
    const Strategy& s = strategies[index];
    if (!wanted(index)) continue;
    std::vector<Row> runs;
    for (int i = 0; i < reps; ++i) runs.push_back(Measure(s.name, s.threads, s.run));
    std::sort(runs.begin(), runs.end(), [](const Row& a, const Row& b) { return a.wall_ms < b.wall_ms; });
    rows.push_back(std::move(runs[runs.size() / 2]));
  }
  return rows;
}

void Report(const std::vector<Row>& rows) {
  PrintHeader();
  for (const Row& r : rows) PrintRow(r);
  std::printf("\n");
  for (std::size_t i = 0; i < rows.size() && i < 2; ++i) PrintPhases(rows[i], 3);
}

// --- the two scenarios ---------------------------------------------------------------------------

int Local(const std::vector<std::string>& args) {
  const std::size_t n = args.size() > 0 ? std::stoul(args[0]) : 20;
  const int delay_ms = args.size() > 1 ? std::stoi(args[1]) : 300;
  const std::size_t threads = args.size() > 2 ? std::stoul(args[2]) : 4;
  const int reps = args.size() > 3 ? std::stoi(args[3]) : 3;

  TestServer server;
  server.server().new_task_queue = [] { return new httplib::ThreadPool(64); };  // never the bottleneck
  server.server().Post("/", [delay_ms](const httplib::Request&, httplib::Response& res) {
    std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    res.set_content("{}", "application/json");
  });
  server.Start();

  std::vector<http::PostRequest> requests(n, http::PostRequest{server.Url("/"), {{"Content-Type", "application/json"}}, "{}"});
  http::Options options;
  const auto version = config::Get("BENCH_VERSION");  // "any": let libcurl choose
  options.version = version && *version == "any" ? http::Version::kAny : http::Version::kHttp1_1;
  std::printf("local server, %zu requests, %d ms per answer, %zu threads, median of %d runs\n\n", n, delay_ms, threads, reps);
  Report(RunAll(requests, threads, options, reps));
  return 0;
}

int Llm(const std::vector<std::string>& args) {
  const std::size_t n = args.size() > 0 ? std::stoul(args[0]) : 8;
  const std::size_t threads = args.size() > 1 ? std::stoul(args[1]) : 4;
  const std::string version = args.size() > 2 ? args[2] : "any";
  const std::vector<std::string> rest(args.begin() + (std::min<std::size_t>)(3, args.size()), args.end());

  const auto candidates = config::ConfigCandidates(config::ExeDirectory(), std::filesystem::current_path());
  const options::Result parsed = options::Parse(rest, candidates, std::cout, std::cerr);
  if (parsed.exit_code) return *parsed.exit_code;
  const config::Values& s = parsed.options.values;

  llm::Request question;
  question.max_tokens = 64;
  llm::Endpoint endpoint;
  std::vector<http::PostRequest> requests;
  for (std::size_t i = 0; i < n; ++i) {
    question.prompt = "Reply with only the number " + std::to_string(i) + ".";
    llm::HttpRequest r;
    if (s.at("llm_provider") == "anthropic") {
      question.model = s.at("anthropic_model");
      endpoint = {s.at("anthropic_base_url"), s.at("anthropic_api_key")};
      r = llm::anthropic::BuildRequest(endpoint, question);
    } else {
      question.model = s.at("openai_model");
      endpoint = {s.at("openai_base_url"), s.at("openai_api_key")};
      r = llm::openai_chat::BuildRequest(endpoint, question);
    }
    requests.push_back({r.url, r.headers, r.body});
  }

  http::Options options;
  options.version = version == "1.1" ? http::Version::kHttp1_1 : version == "2" ? http::Version::kHttp2 : http::Version::kAny;
  const curl_version_info_data* info = curl_version_info(CURLVERSION_NOW);
  std::printf("libcurl %s, HTTP/2 %s\n", info->version, (info->features & CURL_VERSION_HTTP2) ? "built in" : "NOT built in");
  std::printf("%s, %zu questions, %zu threads, HTTP version setting: %s\n\n", s.at("llm_provider").c_str(), n, threads, version.c_str());
  const auto reps = config::Get("BENCH_REPS");  // runs per strategy; the median is shown
  Report(RunAll(requests, threads, options, reps ? std::stoi(*reps) : 1));
  return 0;
}

// --- what the services say about HTTP/2 -----------------------------------------------------------------

const char* const kServices[][2] = {
    {"api.openai.com", "https://api.openai.com/v1/chat/completions"},
    {"api.anthropic.com", "https://api.anthropic.com/v1/messages"},
    {"openrouter.ai", "https://openrouter.ai/api/v1/chat/completions"},
};

struct ProbeText {
  std::vector<std::string> lines;
};

int ProbeDebug(CURL*, curl_infotype type, char* data, size_t size, void* user) {
  if (type != CURLINFO_TEXT) return 0;
  std::string line(data, size);
  while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
  for (const char* wanted : {"ALPN", "CONCURRENT", "MAX_", "GOAWAY", "multiplex"}) {
    if (line.find(wanted) != std::string::npos) {
      static_cast<ProbeText*>(user)->lines.push_back(line);
      break;
    }
  }
  return 0;
}

std::size_t ProbeHeader(char* data, std::size_t size, std::size_t count, void* user) {
  std::string line(data, size * count);
  std::string lower = line;
  std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  if (lower.rfind("alt-svc:", 0) == 0 || lower.rfind("server:", 0) == 0) {
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
    static_cast<ProbeText*>(user)->lines.push_back(line);
  }
  return size * count;
}

std::size_t Ignore(char*, std::size_t size, std::size_t count, void*) { return size * count; }

int Probe() {
  curl_global_trace("http/2");  // makes libcurl log what the server says in its SETTINGS frame
  for (const auto& service : kServices) {
    ProbeText text;
    CURL* easy = curl_easy_init();
    curl_slist* headers = curl_slist_append(nullptr, "Content-Type: application/json");
    curl_easy_setopt(easy, CURLOPT_URL, service[1]);
    curl_easy_setopt(easy, CURLOPT_POSTFIELDS, "{}");
    curl_easy_setopt(easy, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(easy, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
    curl_easy_setopt(easy, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(easy, CURLOPT_DEBUGFUNCTION, ProbeDebug);
    curl_easy_setopt(easy, CURLOPT_DEBUGDATA, &text);
    curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION, ProbeHeader);
    curl_easy_setopt(easy, CURLOPT_HEADERDATA, &text);
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, Ignore);
    curl_easy_setopt(easy, CURLOPT_TIMEOUT_MS, 15000L);
    const CURLcode code = curl_easy_perform(easy);
    long status = 0, version = 0;
    curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &status);
    curl_easy_getinfo(easy, CURLINFO_HTTP_VERSION, &version);
    std::printf("%s: %s, HTTP version code %ld (3 = HTTP/2), status %ld\n", service[0], code == CURLE_OK ? "ok" : curl_easy_strerror(code), version, status);
    for (const std::string& line : text.lines) std::printf("    %s\n", line.c_str());
    curl_slist_free_all(headers);
    curl_easy_cleanup(easy);
  }
  return 0;
}

int Idle() {
  std::mutex out;
  std::vector<std::thread> threads;
  for (const auto& service : kServices) {
    threads.emplace_back([&out, url = std::string(service[1]), name = std::string(service[0])] {
      http::Session session;
      std::string line = name + ":";
      for (const int pause : {0, 5, 30, 90}) {
        std::this_thread::sleep_for(std::chrono::seconds(pause));
        try {
          const http::Response r = session.Post(url, {{"Content-Type", "application/json"}}, "{}");
          line += " after " + std::to_string(pause) + " s: " + (r.timing.new_connections ? "new connection" : "same connection") + " (" + r.timing.protocol + ");";
        } catch (const std::exception& e) {
          line += " after " + std::to_string(pause) + " s: " + e.what() + ";";
        }
      }
      const std::lock_guard<std::mutex> lock(out);
      std::printf("%s\n", line.c_str());
    });
  }
  for (auto& t : threads) t.join();
  return 0;
}

struct StreamStats {
  std::size_t chunks = 0;
  std::size_t bytes = 0;
  Clock::time_point first{};
  Clock::time_point last{};
};

std::size_t StreamWrite(char*, std::size_t size, std::size_t count, void* user) {
  auto* stats = static_cast<StreamStats*>(user);
  const auto now = Clock::now();
  if (stats->chunks == 0) stats->first = now;
  stats->last = now;
  ++stats->chunks;
  stats->bytes += size * count;
  return size * count;
}

int Stream(const std::vector<std::string>& args) {
  const auto candidates = config::ConfigCandidates(config::ExeDirectory(), std::filesystem::current_path());
  const options::Result parsed = options::Parse(args, candidates, std::cout, std::cerr);
  if (parsed.exit_code) return *parsed.exit_code;
  const config::Values& s = parsed.options.values;
  const bool anthropic = s.at("llm_provider") == "anthropic";
  llm::Request question;
  question.max_tokens = 200;
  question.prompt = "Count from 1 to 20, one number per line.";
  llm::HttpRequest r;
  if (anthropic) {
    question.model = s.at("anthropic_model");
    r = llm::anthropic::BuildRequest({s.at("anthropic_base_url"), s.at("anthropic_api_key")}, question);
  } else {
    question.model = s.at("openai_model");
    r = llm::openai_chat::BuildRequest({s.at("openai_base_url"), s.at("openai_api_key")}, question);
  }
  json::Value body = json::Parse(r.body);
  body["stream"] = true;
  const std::string text = json::Dump(body);

  StreamStats stats;
  CURL* easy = curl_easy_init();
  curl_slist* headers = nullptr;
  for (const auto& h : r.headers) headers = curl_slist_append(headers, (h.first + ": " + h.second).c_str());
  curl_easy_setopt(easy, CURLOPT_URL, r.url.c_str());
  curl_easy_setopt(easy, CURLOPT_POSTFIELDS, text.c_str());
  curl_easy_setopt(easy, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(easy, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);
  curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, StreamWrite);
  curl_easy_setopt(easy, CURLOPT_WRITEDATA, &stats);
  curl_easy_setopt(easy, CURLOPT_TIMEOUT_MS, 60000L);
  const auto start = Clock::now();
  const CURLcode code = curl_easy_perform(easy);
  long status = 0, version = 0;
  char* type = nullptr;
  curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &status);
  curl_easy_getinfo(easy, CURLINFO_HTTP_VERSION, &version);
  curl_easy_getinfo(easy, CURLINFO_CONTENT_TYPE, &type);
  auto ms = [&](Clock::time_point t) { return std::chrono::duration<double, std::milli>(t - start).count(); };
  std::printf("%s: %s, HTTP version code %ld (3 = HTTP/2), status %ld, content-type %s\n", s.at("llm_provider").c_str(),
              code == CURLE_OK ? "ok" : curl_easy_strerror(code), version, status, type ? type : "?");
  std::printf("%zu pieces of data, %zu bytes; the first arrived after %.0f ms, the last after %.0f ms\n", stats.chunks, stats.bytes,
              stats.chunks ? ms(stats.first) : 0.0, stats.chunks ? ms(stats.last) : 0.0);
  curl_slist_free_all(headers);
  curl_easy_cleanup(easy);
  return 0;
}

// --- HTTP/2 multiplexing on a real service --------------------------------------------------------------

std::vector<http::PostRequest> Questions(const config::Values& s, std::size_t n) {
  llm::Request question;
  question.max_tokens = 16;
  std::vector<http::PostRequest> requests;
  for (std::size_t i = 0; i < n; ++i) {
    question.prompt = "Reply with only the number " + std::to_string(i) + ".";
    llm::HttpRequest r;
    if (s.at("llm_provider") == "anthropic") {
      question.model = s.at("anthropic_model");
      r = llm::anthropic::BuildRequest({s.at("anthropic_base_url"), s.at("anthropic_api_key")}, question);
    } else {
      question.model = s.at("openai_model");
      r = llm::openai_chat::BuildRequest({s.at("openai_base_url"), s.at("openai_api_key")}, question);
    }
    requests.push_back({r.url, r.headers, r.body});
  }
  return requests;
}

struct MuxRun {
  double wall_ms = 0;
  long connections = 0;           // new connections the requests made
  std::size_t max_overlap = 0;    // the most requests that were waiting for an answer at the same time
  std::size_t ok = 0;             // status 200
  std::size_t throttled = 0;      // status 429: the service said "too many requests"
  std::size_t failed = 0;         // another status, or no answer
  double p50_ms = 0;
  double p95_ms = 0;
};

MuxRun RunMux(const std::vector<http::PostRequest>& requests, http::Version version) {
  struct Span {
    double start = 0;
    double end = 0;
    long status = 0;
    long connections = 0;
  };
  boost::asio::io_context io;
  http::Options options;
  options.version = version;
  http::CurlThreadClient client(options);
  std::vector<Span> spans(requests.size());
  const auto t0 = Clock::now();
  auto since = [&](Clock::time_point t) { return std::chrono::duration<double, std::milli>(t - t0).count(); };
  auto job = [&](std::size_t i) -> boost::asio::awaitable<void> {
    spans[i].start = since(Clock::now());
    try {
      const http::Response r = co_await client.Post(requests[i]);
      spans[i].status = r.status;
      spans[i].connections = r.timing.new_connections;
    } catch (const std::exception&) {
    }
    spans[i].end = since(Clock::now());
  };
  for (std::size_t i = 0; i < requests.size(); ++i) boost::asio::co_spawn(io, job(i), boost::asio::detached);
  io.run();

  MuxRun run;
  run.wall_ms = since(Clock::now());
  std::vector<double> durations;
  std::vector<std::pair<double, int>> events;  // (time, +1 for a request that starts, -1 for one that ends)
  for (const Span& span : spans) {
    run.connections += span.connections;
    if (span.status == 200) ++run.ok;
    else if (span.status == 429) ++run.throttled;
    else ++run.failed;
    durations.push_back(span.end - span.start);
    events.push_back({span.start, +1});
    events.push_back({span.end, -1});
  }
  std::sort(durations.begin(), durations.end());
  run.p50_ms = durations[durations.size() / 2];
  run.p95_ms = durations[(durations.size() * 95) / 100 < durations.size() ? (durations.size() * 95) / 100 : durations.size() - 1];
  std::sort(events.begin(), events.end());  // at the same time an end (-1) comes before a start (+1)
  int now = 0;
  for (const auto& event : events) {
    now += event.second;
    run.max_overlap = (std::max)(run.max_overlap, static_cast<std::size_t>(now));
  }
  return run;
}

int Mux(const std::vector<std::string>& args) {
  std::vector<std::size_t> counts = {12, 50, 100};
  std::vector<std::string> rest = args;
  if (!rest.empty() && rest[0].rfind("--", 0) != 0) {
    counts.clear();
    std::string list = rest[0];
    for (std::size_t from = 0; from <= list.size();) {
      const std::size_t comma = list.find(',', from);
      counts.push_back(std::stoul(list.substr(from, comma == std::string::npos ? std::string::npos : comma - from)));
      if (comma == std::string::npos) break;
      from = comma + 1;
    }
    rest.erase(rest.begin());
  }
  const auto candidates = config::ConfigCandidates(config::ExeDirectory(), std::filesystem::current_path());
  const options::Result parsed = options::Parse(rest, candidates, std::cout, std::cerr);
  if (parsed.exit_code) return *parsed.exit_code;
  const config::Values& s = parsed.options.values;
  const auto reps_setting = config::Get("BENCH_REPS");
  const int reps = reps_setting ? std::stoi(*reps_setting) : 3;

  std::printf("%s, one CurlThreadClient, median of %d runs\n\n", s.at("llm_provider").c_str(), reps);
  std::printf("%5s %-9s %9s %6s %8s %5s %5s %5s %8s %8s\n", "N", "version", "total ms", "conns", "overlap", "ok", "429", "fail", "p50 ms", "p95 ms");
  for (const std::size_t n : counts) {
    const std::vector<http::PostRequest> requests = Questions(s, n);
    for (const http::Version version : {http::Version::kHttp1_1, http::Version::kHttp2}) {
      std::vector<MuxRun> runs;
      for (int i = 0; i < reps; ++i) runs.push_back(RunMux(requests, version));
      std::sort(runs.begin(), runs.end(), [](const MuxRun& a, const MuxRun& b) { return a.wall_ms < b.wall_ms; });
      const MuxRun& r = runs[runs.size() / 2];
      std::printf("%5zu %-9s %9.0f %6ld %8zu %5zu %5zu %5zu %8.0f %8.0f\n", n, version == http::Version::kHttp2 ? "HTTP/2" : "HTTP/1.1", r.wall_ms,
                  r.connections, r.max_overlap, r.ok, r.throttled, r.failed, r.p50_ms, r.p95_ms);
    }
  }
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  std::setvbuf(stdout, nullptr, _IONBF, 0);  // keep the lines that were printed before a crash
  const std::vector<std::string> args(argv + 1, argv + argc);
  if (args.empty() || (args[0] != "local" && args[0] != "llm" && args[0] != "probe" && args[0] != "idle" && args[0] != "stream" && args[0] != "mux")) {
    std::fprintf(stderr,
                 "usage: curl_bench local [N] [DELAY_MS] [THREADS] [REPS]\n       curl_bench llm [N] [THREADS] [any|1.1|2] [options]\n"
                 "       curl_bench probe | idle | stream [options]\n       curl_bench mux [N1,N2,...] [options]\n");
    return 2;
  }
  const std::vector<std::string> rest(args.begin() + 1, args.end());
  try {
    if (args[0] == "probe") return Probe();
    if (args[0] == "idle") return Idle();
    if (args[0] == "stream") return Stream(rest);
    if (args[0] == "mux") return Mux(rest);
    return args[0] == "local" ? Local(rest) : Llm(rest);
  } catch (const std::exception& e) {
    std::fprintf(stderr, "error: %s\n", e.what());
    return 1;
  }
}
