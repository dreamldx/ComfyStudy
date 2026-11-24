// Measures the ways to send many requests with libcurl:
//   curl_bench local [N] [DELAY_MS] [THREADS] [REPS]
//       N requests to a small server on this machine that answers each one after DELAY_MS
//       milliseconds (a stand-in for a model that takes a while to answer); plain HTTP/1.1.
//   curl_bench llm [N] [THREADS] [VERSION] [config options...]
//       N short questions to the service that the settings (config file, environment) name.
//       VERSION is any, 1.1 or 2.
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <curl/curl.h>

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
  };
  std::vector<Row> rows;
  const auto only = config::Get("BENCH_ONLY");  // run just one strategy (for debugging)
  for (std::size_t index = 0; index < strategies.size(); ++index) {
    const Strategy& s = strategies[index];
    if (only && std::stoul(*only) != index) continue;
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

}  // namespace

int main(int argc, char** argv) {
  std::setvbuf(stdout, nullptr, _IONBF, 0);  // keep the lines that were printed before a crash
  const std::vector<std::string> args(argv + 1, argv + argc);
  if (args.empty() || (args[0] != "local" && args[0] != "llm")) {
    std::fprintf(stderr, "usage: curl_bench local [N] [DELAY_MS] [THREADS] [REPS]\n       curl_bench llm [N] [THREADS] [any|1.1|2] [options]\n");
    return 2;
  }
  const std::vector<std::string> rest(args.begin() + 1, args.end());
  try {
    return args[0] == "local" ? Local(rest) : Llm(rest);
  } catch (const std::exception& e) {
    std::fprintf(stderr, "error: %s\n", e.what());
    return 1;
  }
}
