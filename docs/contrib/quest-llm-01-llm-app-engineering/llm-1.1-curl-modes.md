# llm-1.1 libcurl 三种用法的对比（easy、multi、share）

- 状态：已验证 ｜ 类型：支线 ｜ 分支：`quest/llm-01-llm-app-engineering`（支线不打 tag，用 `git log quest/llm-01-llm-app-engineering --grep="^chapter-llm-1.1"` 定位本章提交）
- 所属支线：[支线 llm-01：LLM 应用工程](README.md)
- 可开始于：主线 [1.1 HTTP 与 JSON](../../tutorial/phase-01-llm-chat/1.1-http-json.md)
- 一句话目标：在同一批 LLM 请求上测出 libcurl 的 easy、multi、share 三种用法的差别（各阶段耗时、连接数、总耗时），开启 HTTP/2 并比较它和 HTTP/1.1 多连接的区别，最后选定机器人默认用“线程池加 easy”

## 1. 本章目标与成果

机器人要一次问很多个问题：画图前要让模型改写提示词，Discord 里可能有好几个人同时发话。主线 1.1 的 `http::Post` 一次只发一个请求，每次还新建一个 libcurl 句柄。这一章把 libcurl 的三种用法放在同一批请求上量一遍，再决定机器人用哪一种。

做完后你会得到：

- 每个请求各阶段的耗时：域名解析、建立连接、TLS 握手、首字节、总耗时，以及这个请求有没有建立新连接。
- `http::Session`：保留一个句柄，连续请求时复用连接（easy 接口）。
- `http::PostMany`：在一个线程里同时发出多个请求（multi 接口）。
- `http::Share`：让多个线程的句柄共享缓存（share 接口）。
- libcurl 的 HTTP/2 支持（vcpkg 的 `curl[http2]`），以及 HTTP/2 和 HTTP/1.1 多连接的比较。
- 一个基准程序 `curl_bench`，在本机服务器和真实的模型服务上测各种发送方式。
- 结论：机器人默认用“线程池加 easy”。理由和数据在第 2、5、7 节。

本章只测了 POST 一个完整回答的情形，没有做限流（429）的实验；流式输出留给 llm-1.4。

## 2. 背景与概念

**三种用法。** libcurl 提供三组接口：

- easy：`curl_easy_perform` 发一个请求，发完才返回。一个线程同一时刻只能处理一个 easy 句柄的请求。
- multi：`curl_multi_*` 把许多个 easy 句柄交给一个 multi 句柄，在一个线程里同时推进。
- share：`curl_share_*` 不发请求，只是一个让多个句柄共用缓存的对象。

**一个请求花在哪里。** 一次 HTTPS 请求要先解析域名，再建立 TCP 连接，再做 TLS 握手，之后才发请求、等回答。`curl_easy_getinfo` 能读出各个节点的时间，这些时间都从请求开始算起，是累计值，一个阶段自己花的时间是它和上一个节点的差。复用已有的连接时，前三个阶段都是 0。

**HTTP/1.1 和 HTTP/2。**

- HTTP/1.1：一条连接同一时刻只处理一个请求。想要并发就得开多条连接，每条都要付出建立连接和 TLS 握手的代价，服务端和网关也会限制同一个客户端的连接数。
- HTTP/2：一条连接里可以同时有许多个“流”，每个流是一个请求和它的回答，数据按帧交错传输。同一时刻能有多少个流由服务端通过 `SETTINGS_MAX_CONCURRENT_STREAMS` 告知。浏览器和 libcurl 通过 TLS 握手里的 ALPN 协商用哪一个版本。
- HTTP/2 只有在多个请求走同一条连接时才有意义。libcurl 里只有同一个 multi 句柄里的请求会共用连接来多路复用；一个线程一个 easy 句柄的写法，每个线程各开各的连接，用不上 HTTP/2 的多路复用。

**并发度比连接复用重要。** 大模型回答一个问题要几百毫秒到几十秒，建立一条连接只要十几到几十毫秒。并发度对总耗时的影响远大于连接复用，所以本章主要比能不能同时发，以及为此付出多少代码和风险。

## 3. 本章改动

本章的代码在支线分支 `quest/llm-01-llm-app-engineering` 上，不合入 main，也不打 tag。提交信息以 `chapter-llm-1.1-curl-modes:` 开头，`git log quest/llm-01-llm-app-engineering --grep="^chapter-llm-1.1"` 可以找到。本章有两个提交：

1. 库代码、测试和 HTTP/2：
   - `vcpkg.json`：curl 加上 `http2` 特性（会多装 nghttp2）。
   - `src/http_client.h`、`src/http_client.cpp`：新增 `Timing`、`Version`、`Session`、`Share`、`PostMany`；`Options` 新增 `version` 和 `share`；`Response` 新增 `timing`。
   - `tests/test_http_modes.cpp`、`tests/CMakeLists.txt`：9 个新测试。
2. 基准程序：
   - `bench/curl_bench.cpp`、`CMakeLists.txt`：新目标 `curl_bench`，不是机器人本身的一部分。
   - `scripts/check-build.ps1`：编译和链接命令的数量随之改成 20 和 12。

主线 1.1 的 `http::Post` 的调用方式没有变，已有的测试照常通过。

## 4. 关键代码讲解

### 4.1 读出各阶段的耗时

`Timing` 放在 `Response` 里，每个请求都带着：

<!-- src: src/http_client.h#L23-L28 -->
```cpp
// How long each phase of a request took, as libcurl reports it. The times are cumulative: each one
// counts from the start of the request, so the time a phase took on its own is the difference to
// the one before it. A phase that did not happen (for example the connection of a reused
// connection) is 0.
struct Timing {
  std::chrono::microseconds name_lookup{0};  // the domain name has been resolved
```

填充它只要几个 `curl_easy_getinfo`，`_T` 结尾的选项给出整数微秒，不会有浮点误差：

<!-- src: src/http_client.cpp#L128-L144 -->
```cpp
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
```

### 4.2 easy：保留句柄就能复用连接

`Session` 把一个句柄留在对象里。连接在句柄的缓存里，下一次请求同一个主机时直接用：

<!-- src: src/http_client.cpp#L151-L157 -->
```cpp
struct Session::Impl {
  // The Transfer comes first, so it is destroyed AFTER the handle: while a handle is cleaned up,
  // libcurl may still touch the buffers that its options point to.
  Transfer transfer;
  EasyPtr easy;
  Options options;
};
```

<!-- src: src/http_client.cpp#L168-L176 -->
```cpp
Response Session::Post(const std::string& url, const std::vector<Header>& headers,
                       const std::string& body) {
  Transfer& transfer = impl_->transfer;
  CURL* handle = impl_->easy.get();
  transfer.Configure(handle, url, headers, body, impl_->options);
  const CURLcode code = curl_easy_perform(handle);
  if (code != CURLE_OK) throw Error(transfer.Failure(code));
  return CollectResponse(handle, std::move(transfer.response_body));
}
```

`Impl` 里 `transfer` 写在 `easy` 前面是有原因的，见第 8 节。

测试里用 `new_connections` 和 `connect` 确认复用：

<!-- src: tests/test_http_modes.cpp#L62-L77 -->
```cpp
TEST(HttpTiming, PostMakesANewConnectionEveryTimeButASessionReusesIt) {
  SlowServer slow(0);
  const std::string url = slow.server.Url("/");
  const std::vector<http::Header> headers = {{"Content-Type", "text/plain"}};

  EXPECT_EQ(http::Post(url, headers, "a").timing.new_connections, 1);
  EXPECT_EQ(http::Post(url, headers, "b").timing.new_connections, 1);

  http::Session session;
  const http::Response first = session.Post(url, headers, "a");
  const http::Response second = session.Post(url, headers, "b");
  EXPECT_EQ(first.timing.new_connections, 1);
  EXPECT_EQ(second.timing.new_connections, 0) << "the connection of the first request was reused";
  EXPECT_EQ(second.timing.connect.count(), 0) << "a reused connection has no connect phase";
  EXPECT_EQ(second.body, "b") << "the answers do not get mixed up";
}
```

### 4.3 multi：一个线程里同时发许多个请求

`PostMany` 的主循环是 multi 接口的标准写法：`curl_multi_perform` 推进所有传输，`curl_multi_info_read` 取出完成的，`curl_multi_poll` 在没事可做时睡觉。每个句柄用 `CURLOPT_PRIVATE` 记着自己对应第几个请求，这样结果可以按请求的顺序放回去，一个请求失败也不会影响别的请求：

<!-- src: src/http_client.cpp#L291-L331 -->
```cpp
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
```

`max_in_flight` 限制同时进行的请求数：一个请求完成，就从剩下的请求里补一个进来。

HTTPS 且没有指定 HTTP/1.1 时，每个句柄还设了 `CURLOPT_PIPEWAIT`，让后来的请求等第一条连接，看它是不是 HTTP/2，是的话就走同一条连接：

<!-- src: src/http_client.cpp#L269-L277 -->
```cpp
        // Over TLS the server says in the handshake whether it speaks HTTP/2. Then it is better to
        // wait for the first connection and send the other requests over it than to open one
        // connection for each. Over plain http libcurl cannot know that before the first answer
        // has arrived, so waiting would only serialize the requests: 8 requests that take 200 ms
        // each needed 421 ms instead of 218 ms.
        const bool tls = request.url.rfind("https://", 0) == 0;
        if (tls && options.version != Version::kHttp1_1) {
          Set(curl_easy_setopt(job->easy.get(), CURLOPT_PIPEWAIT, 1L), "PIPEWAIT");
        }
```

明文 http 不能设它，见第 8 节。

测试用一个固定延迟 200 毫秒的服务器确认并发上限确实生效：

<!-- src: tests/test_http_modes.cpp#L98-L113 -->
```cpp
TEST(PostMany, TheRequestsRunAtTheSameTimeButNotMoreThanAllowed) {
  SlowServer slow(kDelayMs);
  const auto requests = Requests(slow.server.Url("/"), 8);

  auto start = Clock::now();
  http::PostMany(requests, 8);
  const auto all_at_once = Since(start);
  EXPECT_LT(all_at_once, std::chrono::milliseconds(3 * kDelayMs / 2)) << "8 requests in one round, also with the default version setting";

  start = Clock::now();
  http::PostMany(requests, 4);
  const auto four_at_a_time = Since(start);
  EXPECT_GE(four_at_a_time, std::chrono::milliseconds(2 * kDelayMs)) << "8 requests in two rounds";
  EXPECT_LT(four_at_a_time, std::chrono::milliseconds(3 * kDelayMs + kDelayMs / 2));
}

```

multi 还有另一种驱动方式 `curl_multi_socket_action`：libcurl 通过回调告诉你要监视哪些套接字和什么时候超时，你把它们放进自己的事件循环（epoll、IOCP、asio 之类），事件到了再通知 libcurl。本章没有用它，因为机器人的并发不需要和别的事件循环合在一起；`curl_multi_poll` 已经够用。把它接进 Boost.Asio 见 llm-1.3。

### 4.4 share：让多个线程的句柄共享缓存

share 对象本身不是线程安全的，多个线程的句柄同时访问它里面的缓存时，libcurl 会调用你给的加锁和解锁回调，锁要自己提供。这里每种共享数据一把锁：

<!-- src: src/http_client.cpp#L196-L201 -->
```cpp
void LockShare(CURL*, curl_lock_data data, curl_lock_access, void* user) {
  static_cast<ShareLocks*>(user)->at(static_cast<std::size_t>(data)).lock();
}
void UnlockShare(CURL*, curl_lock_data data, void* user) {
  static_cast<ShareLocks*>(user)->at(static_cast<std::size_t>(data)).unlock();
}
```

<!-- src: src/http_client.cpp#L209-L223 -->
```cpp
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
```

这里没有共享连接缓存，原因见第 8 节：加上它之后，多线程运行时在 libcurl 内部崩溃过。

### 4.5 线程池加 easy

基准程序里的线程池很简单：几个线程用一个原子计数器领下一个请求，每个请求用 easy 接口发出去。三种变体只差“每次新建句柄”、“共用一个 `Share`”、“每个线程一个 `Session`”：

<!-- src: bench/curl_bench.cpp#L64-L88 -->
```cpp
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
```

## 5. 构建与运行

在支线分支上构建，基准程序是单独的目标 `curl_bench`：

```powershell
git switch quest/llm-01-llm-app-engineering
pwsh -File scripts/dev.ps1 cmake --preset windows-release
pwsh -File scripts/dev.ps1 cmake --build --preset windows-release
build\windows-release\bin\curl_bench.exe local 20 300 4 3
```

`local` 的参数依次是：请求数、服务器对每个请求等待的毫秒数、线程数、重复次数（取总耗时居中的一次）。本机服务器用 cpp-httplib，每个请求固定等 300 毫秒再回答，模拟大模型思考的时间；它只说 HTTP/1.1，结果不依赖网络，可以复现。

对真实服务测量时参数依次是请求数、线程数、HTTP 版本（`any`、`1.1`、`2`），其余的选项和程序本身一样读 `config.ini`；环境变量 `BENCH_REPS` 设置重复次数：

```powershell
$env:BENCH_REPS = 2
build\windows-release\bin\curl_bench.exe llm 12 4 1.1
build\windows-release\bin\curl_bench.exe llm 12 4 2
```

每个问题都是“只回复数字 N”，`max_tokens` 为 64，回答很短。

### 5.1 本机服务器：20 个请求，每个等 300 毫秒，4 个线程

```text
local server, 20 requests, 300 ms per answer, 4 threads, median of 3 runs

strategy                             total ms   avg req ms    new conns  threads     ok  protocol
sequential, new handle each              6201          310           20        1  20/20  HTTP/1.1
sequential, one Session                  6211          311            1        1  20/20  HTTP/1.1
pool, new handle each                    1554          310           20        4  20/20  HTTP/1.1
pool, shared caches (Share)              1553          310           20        4  20/20  HTTP/1.1
pool, a Session per thread               1551          310            4        4  20/20  HTTP/1.1
multi, all in flight                      311          309           20        1  20/20  HTTP/1.1
multi, 4 in flight                       1555          311            4        1  20/20  HTTP/1.1
```

表里每一列的意思：`total ms` 是发完全部请求的总耗时，`avg req ms` 是 libcurl 测出的单个请求平均耗时，`new conns` 是 20 个请求一共新建了几条连接。

- 顺序发 20 个，总耗时就是 20 × 约 310 毫秒，不管用不用 `Session`。
- 4 个线程把总耗时压到约 1/4，每个请求仍要约 310 毫秒。
- 只有 `Session` 版本的连接数降到线程数：每个线程用自己的连接发完所有请求。
- multi 一次性把 20 个全部发出去，总耗时就是一个请求的时间。限制成 4 个同时进行时，和 4 个线程一样。
- 共用 `Share` 的那一行在这里和“每次新建句柄”没有差别：本机没有 TLS，域名也不用解析，没有东西可共享。

换成 8 个线程，线程池的总耗时降到约 930 毫秒，multi 限制成 8 个同时进行也一样。

### 5.2 各阶段的耗时：新句柄和复用句柄

```text
  phases of the first requests (ms since the start of each request): sequential, new handle each
    #        dns  connect      tls first byte    total  conns
    0        0.1      0.4      0.0      306.8    306.8      1
    1        0.0      0.4      0.0      306.6    306.6      1
    2        0.0      0.4      0.0      309.0    309.0      1
  phases of the first requests (ms since the start of each request): sequential, one Session
    #        dns  connect      tls first byte    total  conns
    0        0.0      0.4      0.0      311.7    311.7      1
    1        0.0      0.0      0.0      308.6    308.6      0
    2        0.0      0.0      0.0      310.3    310.3      0
```

新句柄的每个请求都要 0.4 毫秒左右建立连接（本机，不是真实网络）；复用连接的请求，域名解析、建立连接、TLS 都是 0，`conns` 也是 0。

### 5.3 真实服务：12 个问题，4 个线程

HTTP/1.1：

```text
libcurl 8.23.0, HTTP/2 built in
openai, 12 questions, 4 threads, HTTP version setting: 1.1

strategy                             total ms   avg req ms    new conns  threads     ok  protocol
sequential, new handle each              5229          435           12        1  12/12  HTTP/1.1
sequential, one Session                  3902          325            1        1  12/12  HTTP/1.1
pool, new handle each                    1538          482           12        4  12/12  HTTP/1.1
pool, shared caches (Share)              1500          465           12        4  12/12  HTTP/1.1
pool, a Session per thread               1240          360            4        4  12/12  HTTP/1.1
multi, all in flight                     1031          410           12        1  12/12  HTTP/1.1
multi, 4 in flight                       1274          383            4        1  12/12  HTTP/1.1
```

HTTP/2：

```text
libcurl 8.23.0, HTTP/2 built in
openai, 12 questions, 4 threads, HTTP version setting: 2

strategy                             total ms   avg req ms    new conns  threads     ok  protocol
sequential, new handle each              5449          453           12        1  12/12  HTTP/2
sequential, one Session                  5101          425            1        1  12/12  HTTP/2
pool, new handle each                    1311          402           12        4  12/12  HTTP/2
pool, shared caches (Share)              1415          398           12        4  12/12  HTTP/2
pool, a Session per thread               1221          289            4        4  12/12  HTTP/2
multi, all in flight                      658          480            1        1  12/12  HTTP/2
multi, 4 in flight                       1405          448            1        1  12/12  HTTP/2
```

真实服务的单个请求时间在 170 到 1100 毫秒之间波动，每行是两次运行里居中的一次。两张表不要逐行比较，只看趋势：

- 新建连接的阶段：域名解析加连接加 TLS 握手，累计 15 到 30 毫秒（见下一小节）。一个问题本身要 250 到 600 毫秒，连接的代价不到十分之一。
- 线程池和 multi 的收益来自同时发出请求，和连接是否复用关系不大。
- HTTP/1.1 下 multi 同时发 12 个请求，新建了 12 条连接；HTTP/2 下只用 1 条连接。这是能确定的差别。两张表里 multi 的总耗时分别是 1031 和 658 毫秒，但单个请求的平均耗时也不一样（410 和 480 毫秒），我没有重复到足以把 HTTP/2 的影响和服务端延迟的波动分开，所以不能说 HTTP/2 让总耗时降低了多少。

### 5.4 真实服务的各阶段

```text
  phases of the first requests (ms since the start of each request): sequential, new handle each
    #        dns  connect      tls first byte    total  conns
    0       14.6     18.4     28.1      245.1    247.1      1
    1       14.9     18.8     31.0      527.1    531.2      1
    2       15.1     19.2     26.4      525.8    527.9      1
  phases of the first requests (ms since the start of each request): sequential, one Session
    #        dns  connect      tls first byte    total  conns
    0       10.6     14.0     20.4      606.9    609.1      1
    1        0.0      0.0      0.0      356.2    357.1      0
    2        0.0      0.0      0.0      358.2    359.2      0
```

新连接的第一个请求，域名解析约 10 多毫秒，到 TLS 握手完成累计 20 到 30 毫秒；复用连接的请求这几项都是 0，首字节时间只剩服务端处理的时间。

## 6. 验证

- `ctest`：102 个测试通过，其中新增的 9 个在 `test_http_modes` 里：
  - 响应带着各阶段耗时，且前一个节点不晚于后一个节点；明文 http 的 TLS 耗时为 0。
  - `Post` 每次新建连接，`Session` 第二次请求复用连接，且答案不会串。
  - `Session` 在一个请求失败后还能继续用。
  - `PostMany` 的答案按请求顺序返回；同时进行的数量不超过上限（8 个请求、上限 4 个，要两轮）；默认的版本设置下 8 个请求一轮完成；一个请求失败或超时不影响其他请求；空列表返回空结果。
  - 6 个线程共用一个 `Share` 各发 10 个请求，每个都拿到自己的回答。
  - 对明文 http 要求 HTTP/2 时仍然是 HTTP/1.1。
- `scripts/check-build.ps1`：通过（编译 20 条、链接 12 条）。
- 崩溃的检查：基准程序每个策略在 Debug 构建里各跑 30 到 60 次。修好后，不共享连接缓存的 `Share` 连续 60 次没有崩溃；曾经出现的崩溃见第 8 节。
- HTTP/2：`curl_bench llm` 第一行打印 `libcurl 8.23.0, HTTP/2 built in`；指定 `any` 时对真实服务协商出 HTTP/2。

## 7. 技术资料

**版本。** libcurl 8.23.0（vcpkg，Windows 上用 Schannel），nghttp2 1.71.0。加 `http2` 特性之前，`curl_version_info` 报告的库列表里没有 nghttp2，只能说 HTTP/1.1。

**选项。**

| 选项或函数 | 作用 |
|---|---|
| `CURLINFO_NAMELOOKUP_TIME_T` 等 | 各阶段累计微秒数；`APPCONNECT` 是 TLS 握手完成的时间，明文为 0 |
| `CURLINFO_NUM_CONNECTS` | 这次请求新建了几条连接，复用时为 0 |
| `CURLINFO_HTTP_VERSION` | 实际用的版本 |
| `CURLOPT_HTTP_VERSION` | `CURL_HTTP_VERSION_NONE`：让 libcurl 选；`_1_1`：不用 HTTP/2；`_2TLS`：TLS 上用 HTTP/2，否则 HTTP/1.1 |
| `CURLOPT_PIPEWAIT` | 等一条可以多路复用的连接，而不是新开 |
| `CURLOPT_PRIVATE` | 给句柄挂一个指针，multi 的完成消息里能取回 |
| `curl_multi_poll` | 等到有事可做；比自己用 `select` 简单 |
| `CURLOPT_SHARE` | 让句柄使用某个 share 对象 |

**用哪一种。**

| | easy 加线程池 | multi | share |
|---|---|---|---|
| 同时发 N 个请求 | N 个线程 | 1 个线程 | 不是发请求的 |
| 复用连接 | 每个线程用自己的 `Session` | 同一个 multi 里自动复用 | 本机测不出差别，不共享连接 |
| HTTP/2 多路复用 | 用不上，每个线程各开连接 | 能用，同一条连接发多个请求 | 无关 |
| 取消和超时 | 每个请求各自设置 | 每个请求各自设置，但需要自己写循环 | 无关 |
| 代码量 | 最少，主线 1.1 的 `Post` 加一个池子 | 约 100 行状态机，错误处理要自己写 | 约 40 行，外加加锁 |
| 风险 | 线程数要自己限制 | 在回调里不能阻塞，否则其他请求都停下 | 共享连接缓存时崩溃过 |

**被否决的方案。**

- `curl_multi_socket_action`：和自己的事件循环集成时才需要，这里用不上，见 llm-1.3。
- 共享 `CURL_LOCK_DATA_CONNECT`：崩溃，见第 8 节。
- 为了测 HTTP/2 在本机起一个 TLS 加 HTTP/2 的服务器：cpp-httplib 不说 HTTP/2，所以 HTTP/2 只在真实服务上测，数据不能完全复现。

**结论：机器人默认用“线程池加 easy”。**

- 一个模型回答要几百毫秒到几十秒，建立连接只有十几到几十毫秒，所以并发度比复用连接重要得多，而线程池已经提供了并发度。
- 每个线程用自己的 `Session` 还能再省下每个请求十几到几十毫秒的连接时间，几乎没有额外代码。
- multi 在 20 个请求同时发出时总耗时最短，也能用上 HTTP/2；代价是要维护一套状态机，一个请求的取消、超时和错误都要自己处理。机器人现在的并发量（几个用户、几个请求）用不到它。
- share 在测量中没有带来可见的好处，开启连接缓存共享还崩溃过。
- 线程数的上限以后要按服务的限流来定。本章没有测 429，所以先用小的线程数（4 到 8），等遇到限流再回来调。

以后如果出现几百个同时进行的请求，或者流式输出需要在一个线程里同时盯着很多条连接，再考虑换成 multi。

## 8. 常见问题与坑

**段错误：缓冲区比句柄先销毁。** 这是本章遇到的第一个崩溃。`Transfer` 里的请求头列表、请求体和错误缓冲区，是句柄的选项指向的内存。`curl_easy_cleanup` 在关闭连接时还可能碰它们，所以它们必须比句柄晚销毁。C++ 按声明的相反顺序销毁成员，因此 `transfer` 要写在 `easy` 前面（`Session::Impl` 和 `Job` 里都是）。我一开始写反了，基准程序里每 4 到 5 次运行崩溃 1 次，崩溃点是随机的，每个策略都会出现。

**`Share` 的锁也要比 share 句柄晚销毁。** `curl_share_cleanup` 会调用解锁回调，如果这时锁已经销毁，同样崩溃。成员顺序同理：锁在前，句柄在后。

**共享连接缓存会在 libcurl 里崩溃。** 修好上面两个问题之后，只有共享了 `CURL_LOCK_DATA_CONNECT` 的那种策略还会崩溃：Debug 构建里只跑这一个策略 30 次崩 6 次，完整运行 40 次崩 9 次，Release 构建 60 次崩 1 次，崩溃点在 libcurl 内部匹配连接的函数里（`url_match_destination`，读空指针）。把锁换成递归互斥锁没有变化，去掉 `CURL_LOCK_DATA_CONNECT` 之后 Debug 构建连续 60 次没有崩溃。我没有深入 libcurl 查它是库的问题还是用法的问题，选择了绕开：`Share` 只共享域名解析和 TLS 会话，要复用连接就让每个线程用自己的 `Session`。

**明文 http 上设 `CURLOPT_PIPEWAIT` 会把请求串起来。** 设了它，后面的请求会等第一个请求的连接；HTTPS 下握手时就知道服务端是不是 HTTP/2，等一下就能决定；明文 http 要等到第一个回答回来才知道，于是 8 个各需要 200 毫秒的请求用了 421 毫秒而不是 218 毫秒。所以只对 HTTPS 且没有指定 HTTP/1.1 的请求设它。这个问题是测试没通过才发现的。

**`std::min` 在 Windows 上要加括号。** `curl/curl.h` 会间接包含 `windows.h`，它定义了 `min` 和 `max` 宏，`std::min(a, b)` 会编译失败；写成 `(std::min)(a, b)`。

**真实服务的数字有波动。** 同一个问题，同一个服务，单个请求从 170 毫秒到 1100 毫秒都出现过。比较两种发送方式，只看差别明显的部分（连接数、并发带来的倍数），不要比较百分之几十以内的差别。

**HTTP/2 不是 HTTP/1.1 的直接升级。** 同一条 TCP 连接丢一个包，上面所有的流都会等；HTTP/3 用 QUIC 解决了这一点。另外，服务端对一条连接上的并发流数有上限，超过的请求要排队。

## 9. 练习

1. 把 `Session` 和 `PostMany` 组合起来：每个线程用一个 `PostMany`，在线程里同时发多个请求。线程数和每个线程的同时进行数怎么搭配，总耗时最短？
2. 用 `CURLMOPT_MAX_HOST_CONNECTIONS` 限制 multi 对同一个主机的连接数，在 HTTP/1.1 下重新测一遍，看它和 `max_in_flight` 有什么区别。
3. 把 `Share` 加上 `CURL_LOCK_DATA_CONNECT` 之后，用 `curl_bench local 20 50 4 1` 连续运行几十次，复现崩溃。再试着只在线程池里共用一个 `Session`（加互斥锁），比较和每个线程一个 `Session` 的差别。
4. 对真实服务把请求数加到 50，看看线程数从 2 加到 16 时总耗时怎么变；什么时候不再变快？那个时候你遇到的是什么限制？

## 10. 支线

本章节没有可以开启的支线。

## 11. 下一章预告

下一章 llm-1.2 讲 Boost.Asio 的事件循环、定时器和 C++20 协程；llm-1.3 再把 libcurl 的 multi 接进去，两种驱动方式都做。之后的 llm-1.4 是流式输出（SSE）：不等整个回答，边收边显示，会用到这套协程和定时器。
