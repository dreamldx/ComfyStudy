# llm-1.2 libcurl 三种用法：在事件循环里的实现与对比

- 状态：已验证 ｜ 类型：支线 ｜ 分支：`quest/llm-01-llm-app-engineering`（支线不打 tag，用 `git log quest/llm-01-llm-app-engineering --grep="^chapter-llm-1.2"` 定位本章提交）
- 所属支线：[支线 llm-01：LLM 应用工程](README.md)
- 可开始于：主线 [1.1 HTTP 与 JSON](../../tutorial/phase-01-llm-chat/1.1-http-json.md)
- 一句话目标：把 libcurl 的 easy、multi、share 三种用法做成积木（`Timing`、`Session`、`PostMany`、`Share`），再做三种协程实现并排比较：C 线程池加 `Session`、A 一个 curl 线程加 `post`、B `curl_multi_socket_action` 接 Asio；讲 HTTP/1.1 与 HTTP/2 的原理和差别，在本机基准（Release）和真实服务（OpenRouter、Anthropic，OpenAI 官方只记 ALPN）上测出差别，得出机器人默认用哪一种；依赖 llm-1.1

## 1. 本章目标与成果

机器人要一次问很多个问题：画图前要让模型改写提示词，Discord 里可能有好几个人同时发话，一个人的一轮对话里也可能连发几个请求。主线 1.1 的 `http::Post` 一次只发一个请求，每次还新建一个 libcurl 句柄。这一章把 libcurl 的三种用法，和把它们接进事件循环的三种做法，放在同一批请求上量一遍，再决定机器人用哪一种。

做完后你会得到：

- 每个请求各阶段的耗时：域名解析、建立连接、TLS 握手、首字节、总耗时，以及这个请求有没有建立新连接。
- libcurl 三种接口的封装：`http::Session`（easy：保留句柄，复用连接）、`http::PostMany`（multi：一个线程里同时发许多个请求）、`http::Share`（share：让多个线程的句柄共享缓存）。
- 三个协程客户端，签名相同，都是 `awaitable<Response> Post(PostRequest)`：
  - `PoolClient`（下面叫 C）：阻塞的请求放进线程池，每个线程有自己的 `Session`。
  - `CurlThreadClient`（A）：一个专门的线程跑 `curl_multi_poll`，所有请求挂在同一个 multi 句柄上。
  - `SocketClient`（B）：`curl_multi_socket_action`，libcurl 的套接字交给事件循环等，没有额外的线程。
- libcurl 的 HTTP/2 支持（vcpkg 的 `curl[http2]`），以及 HTTP/1.1 和 HTTP/2 的原理与差别。
- 一个基准程序 `curl_bench`：本机服务器上的对比，真实服务（OpenRouter、Anthropic）上的对比，以及四个只读的探测（三家服务对 HTTP/2 怎么说、空闲的连接活多久、流式请求在 HTTP/2 下怎么到达、12 到 300 个请求同时在途时多路复用怎么样）。
- 结论：机器人默认用 A，理由和数据在第 6、7 节。

本章只测了 POST 一个完整回答的情形，没有做限流（429）的实验；流式输出留给 llm-1.4；`Transport` 接口留给 llm-1.3。

## 2. 背景与概念

### 2.1 libcurl 的三种接口

- easy：`curl_easy_perform` 发一个请求，发完才返回。一个线程同一时刻只能处理一个 easy 句柄的请求。
- multi：`curl_multi_*` 把许多个 easy 句柄交给一个 multi 句柄，在一个线程里同时推进。
- share：`curl_share_*` 不发请求，只是一个让多个句柄共用缓存的对象。

### 2.2 一个请求花在哪里

一次 HTTPS 请求要先解析域名，再建立 TCP 连接，再做 TLS 握手，之后才发请求、等回答。`curl_easy_getinfo` 能读出各个节点的时间，这些时间都从请求开始算起，是累计值，一个阶段自己花的时间是它和上一个节点的差。复用已有的连接时，前三个阶段都是 0。

在这台机器上对 OpenRouter 的一次新连接，累计到域名解析是 8 毫秒，到 TCP 连接是 12 毫秒，到 TLS 握手完成是 22 毫秒，之后等模型回答要几百毫秒。建立一条连接只占一次请求很小的一部分。

### 2.3 三个协程客户端的原理

三个客户端要解决的是同一件事：协程发起请求后暂停，线程去做别的事，请求结束时协程在自己的线程上继续。差别在谁等网络、用几个线程。

**C：线程池加 `Session`。** 协程把“发一个阻塞请求”交给线程池（第 4 节的 `OnPool`），池线程里调用 `Session::Post`，请求结束后协程恢复。每个池线程有一个自己的 `Session`，一个 `Session` 一个句柄，连接留在句柄的缓存里。同时在途的请求数不超过线程数，多的要排队。一个请求在 `curl_easy_perform` 里面的时候，那个线程只能等它结束，所以不能被打断。

**A：一个线程跑 multi。** 一个专门的线程循环调用 `curl_multi_perform` 和 `curl_multi_poll`，所有请求都是这个 multi 句柄上的传输。协程发起请求时，把请求放进一个队列，用 `curl_multi_wakeup` 叫醒 curl 线程；请求结束时，curl 线程把结果通过协程所在执行器 `post` 回去。取消一个请求就是把它从 multi 句柄里摘掉。同时在途的请求数没有上限。

**B：`curl_multi_socket_action`，事件循环等套接字。** libcurl 不自己等了，它通过两个回调告诉你：“请监视这个套接字的读或写”，“请在 N 毫秒后叫我”。你把套接字交给事件循环去等，事件到了或时间到了，再调用 `curl_multi_socket_action` 告诉 libcurl。整个过程在事件循环的那一个线程里，没有额外的线程。

三者的差别可以这样记：C 把“阻塞”放进线程，A 把“阻塞”集中到一个线程，B 干脆没有阻塞。

### 2.4 HTTP/1.1 和 HTTP/2

- **HTTP/1.1：** 一条连接同一时刻只处理一个请求，要等回答完整收完，才能发下一个。想要并发就得开多条连接，每条都要付出建立连接和 TLS 握手的代价；服务端和网关也会限制同一个客户端的连接数。
- **HTTP/2：** 一条连接里可以同时有许多个“流”，每个流是一个请求和它的回答，数据切成帧交错传输，互不等待。同一时刻能有多少个流由服务端在握手后的 SETTINGS 帧里用 `MAX_CONCURRENT_STREAMS` 告知。头部用 HPACK 压缩，重复发的头部（`Authorization`、`Content-Type`）只花很少的字节。
- **协商：** 浏览器和 libcurl 在 TLS 握手里用 ALPN 协商用哪个版本。客户端提供 `h2` 和 `http/1.1`，服务端选一个，选 `h2` 就走 HTTP/2。明文 http 没有 TLS，也就没有 ALPN，所以明文 http 总是 HTTP/1.1，本机的测试服务器（cpp-httplib 也不支持 HTTP/2）测不了 HTTP/2，只能用真实服务。
- **流量控制：** 每个流和整条连接各有一个接收窗口，接收方用 `WINDOW_UPDATE` 帧放行更多数据。慢的接收方只会让自己那个流（和整条连接的总量）慢下来，不会让别的流的数据丢失。对 SSE 这类长时间持续的流，窗口决定了发送方领先接收方多远。
- **队头阻塞：** HTTP/2 解决了“一条连接一次一个请求”的阻塞，但所有的流还在同一条 TCP 连接上，一个丢包会让这条连接上所有的流都等重传。HTTP/3（QUIC）用 UDP 绕开这一点。本章不测 HTTP/3，只记录服务有没有宣告它（`Alt-Svc` 头）。
- **谁能享受多路复用：** libcurl 里只有同一个 multi 句柄里的请求会共用一条连接。所以 A 和 B（一个 multi 句柄）在 HTTP/2 下 12 个请求只开一条连接，而 C 每个线程各有各的句柄，各开各的连接，4 个线程就是 4 条，用不上多路复用。

### 2.5 并发度比连接复用重要

大模型回答一个问题要几百毫秒到几十秒，建立一条连接只要十几到几十毫秒。并发度对总耗时的影响远大于连接复用，所以本章主要比能不能同时发，以及为此付出多少代码和风险。

## 3. 本章改动

本章的代码在支线分支 `quest/llm-01-llm-app-engineering` 上，不合入 main，也不打 tag。提交信息以 `chapter-llm-1.2-curl-modes:` 开头，`git log quest/llm-01-llm-app-engineering --grep="^chapter-llm-1.2"` 可以找到。本章有五个提交：

1. 积木：
   - `vcpkg.json`：curl 加上 `http2` 特性（会多装 nghttp2）。
   - `src/http_client.h`、`src/http_client.cpp`：新增 `Timing`、`Version`、`Session`、`Share`、`PostMany`；`Options` 新增 `version` 和 `share`；`Response` 新增 `timing`。
   - `tests/test_http_modes.cpp`：9 个测试。
2. 第一版基准：`bench/curl_bench.cpp`，新目标 `curl_bench`。
3. 三个协程客户端：
   - `src/curl_support.h`：几个文件共用的 libcurl 辅助代码，从 `http_client.cpp` 里搬出来。
   - `src/async_http.h`、`src/async_http.cpp`：`PoolClient`、`CurlThreadClient`、`SocketClient`。
   - `tests/test_async_http.cpp`：23 个测试。
   - `CMakeLists.txt`：`async_http.cpp` 进了核心库，核心库链接 `comfystudy_asio`；`scripts/check-build.ps1`：编译和链接命令的数量随之改成 24 和 15。
4. 基准加上三个协程客户端和三个探测：`curl_bench probe`、`idle`、`stream`。
5. 多路复用的专门测量：`curl_bench mux`。

每个提交同时更新了分支根目录的 `README.md`。主线 1.1 的 `http::Post` 的调用方式没有变，已有的测试照常通过。

## 4. 关键代码讲解

### 4.1 读出各阶段的耗时

`Timing` 放在 `Response` 里，每个请求都带着：

<!-- src: src/http_client.h#L23-L35 -->
```cpp
// How long each phase of a request took, as libcurl reports it. The times are cumulative: each one
// counts from the start of the request, so the time a phase took on its own is the difference to
// the one before it. A phase that did not happen (for example the connection of a reused
// connection) is 0.
struct Timing {
  std::chrono::microseconds name_lookup{0};  // the domain name has been resolved
  std::chrono::microseconds connect{0};      // the TCP connection is established
  std::chrono::microseconds tls{0};          // the TLS handshake is done (0 for plain http)
  std::chrono::microseconds first_byte{0};   // the first byte of the answer has arrived
  std::chrono::microseconds total{0};        // the whole answer has arrived
  long new_connections = 0;                  // 0 when an existing connection was reused
  std::string protocol;                      // "HTTP/1.1", "HTTP/2", ...
};
```

请求结束时从句柄里读出来（`_T` 结尾的是 libcurl 提供的微秒版本，不会丢精度）：

<!-- src: src/curl_support.h#L134-L151 -->
```cpp
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
```

### 4.2 easy：保留句柄就能复用连接

`Session` 把一个句柄留在对象里，连接在句柄的缓存里，下一次请求同一个主机时直接用。主线 1.1 的 `Post` 现在只是“新建一个 `Session`，用一次”：

<!-- src: src/http_client.cpp#L16-L47 -->
```cpp
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
```

`Impl` 里 `transfer` 写在 `easy` 前面是有原因的，见第 8 节。

### 4.3 multi：一个线程里同时发许多个请求

`PostMany` 的主循环是 multi 接口的标准写法：`curl_multi_perform` 推进所有传输，`curl_multi_info_read` 取出完成的，`curl_multi_poll` 在没事可做时睡觉。每个句柄用 `CURLOPT_PRIVATE` 记着自己对应第几个请求，完成的消息里就能找回来。

<!-- src: src/http_client.cpp#L156-L197 -->
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
}
```

`max_in_flight` 限制同时进行的请求数：一个请求完成，就从剩下的请求里补一个进来。

### 4.4 share：让多个线程的句柄共享缓存

`Share` 共享 DNS 缓存和 TLS 会话，不共享连接缓存。多线程使用时 libcurl 会回调加锁和解锁，锁由我们提供：

<!-- src: src/http_client.cpp#L74-L88 -->
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

注释里写的原因见第 8 节：共享连接缓存之后，多线程运行时在 libcurl 内部崩溃过。

### 4.5 从协程里等一个别的线程的结果

三个客户端里 A 和 B 都要把“请求结束了”这件事交给暂停着的协程。`Suspend` 做这件事：

<!-- src: src/async_http.cpp#L28-L48 -->
```cpp
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
```

`async_initiate` 让这个函数成为一个可以 `co_await` 的异步操作：初始化函数拿到 `handler`（协程恢复的入口）、它所在的执行器和取消的入口。我们把它们包进一个 `Done` 函数交给 `start`；`Done` 可以在任何线程里调用，它用 `asio::post` 把恢复协程的动作投回协程自己的执行器（这里一定要 `post`，不能 `dispatch`：`dispatch` 在同一个线程上会立刻运行，协程就在 libcurl 的回调里恢复了，它要是又发起新请求，就是在 libcurl 的回调里调用 libcurl）。`executor_work_guard` 让事件循环在请求在途期间不会以为自己没事做而退出。

### 4.6 C：`PoolClient`

<!-- src: src/async_http.cpp#L67-L111 -->
```cpp
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
```

一共有多少个线程就有多少个 `Session`，`Lease` 在一个请求的期间借出一个，用完放回去；放回去时放在最后，借的时候从最后拿，所以刚用过的 `Session`（连接最热）会被最先再用。`OnPool` 是 llm-1.1 里写的“切到池里干活再回来”。

### 4.7 A：`CurlThreadClient`

协程这一侧把请求放进队列，登记取消的动作，叫醒 curl 线程：

<!-- src: src/async_http.cpp#L242-L266 -->
```cpp
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
```

curl 线程这一侧的循环：每一轮先取出新请求和要取消的请求，加进 multi 句柄或摘掉，然后推进传输、收结果，最后在 `curl_multi_poll` 里睡觉，直到有传输要处理，或者被 `curl_multi_wakeup` 叫醒：

<!-- src: src/async_http.cpp#L167-L204 -->
```cpp
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
```

`curl_multi_wakeup` 是 libcurl 里少数明确可以从别的线程调用的函数，A 的线程安全就靠它加一把保护队列的锁。收完的传输在下半部分处理，和 `PostMany` 里一样。

### 4.8 B：`SocketClient`

libcurl 通过两个回调把工作交给我们：

<!-- src: src/async_http.cpp#L312-L354 -->
```cpp
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
```

`OnSocket` 在 libcurl 要监视或不再监视一个套接字时被调用。第一次见到一个套接字，把它 `assign` 给 Asio 的 `tcp::socket`，之后只用它的 `async_wait`；libcurl 说“不用监视了”时，对它 `release()`，让 Asio 对象放手但不关闭套接字，套接字由 libcurl 自己关。`OnTimer` 是 libcurl 要求的定时：`0` 毫秒要立刻处理，但要 `post` 出去，不能在回调里直接调用 `curl_multi_socket_action`。

监视的具体做法：

<!-- src: src/async_http.cpp#L357-L391 -->
```cpp
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
```

读和写各等一次，等到了就通知 libcurl，然后看这个套接字是否还登记着，登记着就再等。最后一段是 Windows 特有的：连接失败（被拒绝、不可达）在 Windows 上不是“可写”，而是出现在 select 的异常集合里，不额外等 `wait_error`，失败要等到 libcurl 自己的连接超时（默认 10 秒）才会被发现。这个问题是测试里发现的：B 的那一组 6 个测试，不等 `wait_error` 要约 11.9 秒（其中“一个请求失败不影响其他”一项就是 10 秒），等了之后约 1.9 秒。

通知 libcurl 之后，收取完成的传输：

<!-- src: src/async_http.cpp#L395-L415 -->
```cpp
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
```

### 4.9 测试怎么共用

同一组测试用 `TYPED_TEST` 跑遍三个客户端，只有“怎么造客户端”不一样。比如“20 个请求同时在途，各拿各的答案”：

<!-- src: tests/test_async_http.cpp#L100-L114 -->
```cpp
TYPED_TEST(AsyncHttpTest, TwentyRequestsAtTheSameTimeEachGetTheirOwnAnswer) {
  auto client = this->MakeClient();
  std::vector<std::optional<std::string>> bodies(20);
  auto job = [&](int i) -> asio::awaitable<void> {
    const http::Response response = co_await client->Post(this->Request("/slow?ms=300", "answer " + std::to_string(i)));
    bodies[static_cast<std::size_t>(i)] = response.body;
  };
  const auto start = loop::Clock::now();
  for (int i = 0; i < 20; ++i) asio::co_spawn(this->io_, job(i), asio::detached);
  this->io_.run();
  const auto elapsed = loop::Clock::now() - start;
  for (int i = 0; i < 20; ++i) EXPECT_EQ(bodies[static_cast<std::size_t>(i)], "answer " + std::to_string(i));
  EXPECT_GE(elapsed, 300ms);
  EXPECT_LT(elapsed, 1500ms);  // 20 requests one after the other would take 6 seconds
}
```

只有 A 和 B 能做的取消，放在单独的一组里：

<!-- src: tests/test_async_http.cpp#L169-L187 -->
```cpp
TYPED_TEST(InterruptibleTest, CancellingTheCoroutineCancelsTheRequest) {
  auto client = this->MakeClient();
  asio::cancellation_signal signal;
  const auto start = loop::Clock::now();
  auto answer = asio::co_spawn(
      this->io_,
      [&]() -> asio::awaitable<http::Response> { co_return co_await client->Post(this->Request("/slow?ms=1500", "x")); },
      asio::bind_cancellation_slot(signal.slot(), asio::use_future));
  asio::steady_timer later(this->io_, 100ms);
  later.async_wait([&](const boost::system::error_code&) { signal.emit(asio::cancellation_type::all); });
  this->io_.run();
  EXPECT_LT(loop::Clock::now() - start, 1000ms);  // the answer would have taken 1500 ms
  try {
    answer.get();
    FAIL() << "the request was not cancelled";
  } catch (const boost::system::system_error& e) {
    EXPECT_EQ(e.code(), asio::error::operation_aborted);
  }
}
```

基准里把三个客户端放进同一个函数：同时发出所有请求，等所有答案：

<!-- src: bench/curl_bench.cpp#L173-L186 -->
```cpp
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
```

## 5. 构建与运行

在支线分支上构建。基准用 Release 构建，Debug 构建的数字不拿来比较。

```powershell
git switch quest/llm-01-llm-app-engineering
.\scripts\dev.ps1 cmake --preset windows-release
.\scripts\dev.ps1 cmake --build --preset windows-release --target curl_bench
```

### 5.1 本机服务器：20 个请求，每个等 300 毫秒，4 个线程

```powershell
$env:BENCH_REPS = 3
build\windows-release\bin\curl_bench.exe local 20 300 4 3
```

`local` 的参数依次是：请求数、服务器对每个请求等待的毫秒数、线程数、重复次数（取总耗时居中的一次）。本机服务器每个请求固定等 300 毫秒再回答，模拟大模型思考的时间；它只说 HTTP/1.1。表里的线程数包括跑事件循环或调用的那一个。

```
strategy                             total ms   avg req ms    new conns  threads     ok  protocol
sequential, new handle each              6233          311           20        1  20/20  HTTP/1.1
sequential, one Session                  6200          310            1        1  20/20  HTTP/1.1
pool, new handle each                    1549          309           20        4  20/20  HTTP/1.1
pool, shared caches (Share)              1555          311           20        4  20/20  HTTP/1.1
pool, a Session per thread               1554          311            4        4  20/20  HTTP/1.1
multi, all in flight                      310          307           20        1  20/20  HTTP/1.1
multi, 4 in flight                       1549          310            4        1  20/20  HTTP/1.1
async C: pool of Sessions                1552          310            4        5  20/20  HTTP/1.1
async A: one curl thread                  310          308           20        2  20/20  HTTP/1.1
async B: socket_action                    311          307           20        1  20/20  HTTP/1.1
```

- 顺序发 20 个就是 20 个 300 毫秒，复用连接不改变总耗时（省下的只有每个请求零点几毫秒）。
- 线程池 4 个线程：20 个请求要 5 轮，约 1550 毫秒。共享缓存（`Share`）没有带来可见的变化，因为本机连接没有域名解析也没有 TLS 可省。
- multi 全部同时发出：310 毫秒，一个线程。限制成 4 个同时进行，就和 4 个线程的池一样，1549 毫秒：时间取决于同时在途的数量。
- 三个协程客户端：C 的 1552 毫秒和 `pool, a Session per thread` 一样（4 个线程，4 条连接）；A 和 B 的 310、311 毫秒和 `multi, all in flight` 一样，A 比 B 多一个线程，B 只用事件循环的那一个。

### 5.2 真实服务：12 个问题，4 个线程

```powershell
$env:BENCH_REPS = 5
$env:BENCH_ONLY = "4,5,7,8,9"   # 只跑这几种：一个 Session 一个线程的池、multi、C、A、B
build\windows-release\bin\curl_bench.exe llm 12 4 1.1
build\windows-release\bin\curl_bench.exe llm 12 4 2
build\windows-release\bin\curl_bench.exe llm 12 4 1.1 --llm_provider anthropic
build\windows-release\bin\curl_bench.exe llm 12 4 2 --llm_provider anthropic
```

参数依次是请求数、线程数、HTTP 版本（`any`、`1.1`、`2`），其余的选项和程序本身一样读 `config.ini`（`--llm_provider anthropic` 切到 Anthropic）。每个问题都是“只回复数字 N”，`max_tokens` 为 64，回答很短。下面是重复 5 次取居中的一次：

OpenRouter（`openai_base_url` 指向它），毫秒；括号里是新建的连接数：

| 策略 | HTTP/1.1 | HTTP/2 |
|---|---|---|
| 池，每线程一个 Session（4 线程） | 1589（4） | 1176（4） |
| multi，全部同时发出 | 463（12） | 654（1） |
| C：Session 池（4 线程） | 1008（4） | 1079（4） |
| A：一个 curl 线程 | 620（12） | 567（1） |
| B：`socket_action` | 700（12） | 612（1） |

Anthropic：

| 策略 | HTTP/1.1 | HTTP/2 |
|---|---|---|
| 池，每线程一个 Session（4 线程） | 2423（4） | 2443（4） |
| multi，全部同时发出 | 1235（12） | 1242（1） |
| C：Session 池（4 线程） | 2431（4） | 2449（4） |
| A：一个 curl 线程 | 1172（12） | 1170（1） |
| B：`socket_action` | 1192（12） | 1166（1） |

怎么读：

- **连接数是确定的差别：** multi、A、B 在 HTTP/2 下 12 个请求只开 1 条连接，HTTP/1.1 下开 12 条；线程池（C）不论哪个版本都是 4 条，它没有任何东西可以复用。
- **总耗时和 HTTP 版本几乎无关：** Anthropic 上同一个策略 HTTP/1.1 和 HTTP/2 差不到 3%；OpenRouter 上的差别（比如 multi 的 463 对 654）在同一个服务的波动范围内，同一个问题的回答时间从 170 毫秒到 1 秒多都出现过，总耗时又取决于最慢的那个回答。
- **总耗时取决于同时能发几个：** Anthropic 单个回答约 750 毫秒，12 个请求，4 个线程要 3 轮，约 2.4 秒，而 A、B、multi 一次发完，约 1.2 秒。
- **波动有多大，看设计相同的两行：** OpenRouter 上“池，每线程一个 Session”和 C 是同一种做法，分别是 1589 和 1008 毫秒。
- **A 和 B 的差别在这个量级上看不出来**，两个都和一个线程的阻塞 multi 一样快。

### 5.3 三家服务怎么说 HTTP/2

```powershell
build\windows-release\bin\curl_bench.exe probe
```

`probe` 对三家各发一个不带密钥的请求（会被拒绝，返回 401，不花钱），读出协商的协议和服务端的设置：

| 服务 | ALPN | `MAX_CONCURRENT_STREAMS` | `Alt-Svc`（HTTP/3） | `server` 头 |
|---|---|---|---|---|
| `api.openai.com` | `h2` | 256 | `h3=":443"; ma=86400` | cloudflare |
| `api.anthropic.com` | `h2` | 256 | 没有 | cloudflare |
| `openrouter.ai` | `h2` | 256 | `h3=":443"; ma=86400` | cloudflare |

三家都在 Cloudflare 后面，都支持 HTTP/2，服务端允许一条连接上最多 256 个并发的流；libcurl 自己默认再限制到每条连接 100 个。机器人用不到这么多。OpenAI 官方只测到这里：这张表没有用到它的密钥，没有测它的回答时间，也没有测多路复用。

### 5.4 空闲的连接活多久

```powershell
build\windows-release\bin\curl_bench.exe idle
```

对三家各用一个 `Session` 发请求，隔 0、5、30、90 秒再发，看第二次是不是同一条连接。三家的结果一样：隔 5、30、90 秒都还是同一条连接（HTTP/2）。90 秒以内的空闲不需要处理“连接被关了”的情况；更长的空闲我们没有测。据我所知 libcurl 复用一条已经被对方关掉的连接失败时，会自己改用一条新的连接重试，但这一点没有在这里验证。

### 5.5 流式请求在 HTTP/2 下

```powershell
build\windows-release\bin\curl_bench.exe stream --llm_provider anthropic
build\windows-release\bin\curl_bench.exe stream
```

发一个 `stream: true` 的小请求（数 1 到 20），只数收到的数据块和时间，不看内容：

```
anthropic: ok, HTTP version code 3 (3 = HTTP/2), status 200, content-type text/event-stream; charset=utf-8
2 pieces of data, 1814 bytes; the first arrived after 563 ms, the last after 1171 ms
openai: ok, HTTP version code 3 (3 = HTTP/2), status 200, content-type text/event-stream
9 pieces of data, 5640 bytes; the first arrived after 337 ms, the last after 450 ms
```

两家的流式在 HTTP/2 下都能正常到达，类型是 `text/event-stream`。到达的块有多粗，要看服务和中间的网关怎么攒数据：Anthropic 这次只有两大块，OpenRouter 是九块。事件边界的处理留给 llm-1.4，这里只确认流在 HTTP/2 上能用。

### 5.6 多路复用：12 到 300 个请求同时在途

上面 12 个请求只看到“1 条连接”。`mux` 把它单独测一遍：用一个 `CurlThreadClient`，同时发出 N 个请求，分别在 HTTP/1.1 和 HTTP/2 下各做 3 次，取总耗时居中的一次，并且记下三件事：新建了几条连接；同一时刻最多有几个请求在等回答（“同时在途”）；有没有被服务拒绝（429）。

```powershell
$env:BENCH_REPS = 3
build\windows-release\bin\curl_bench.exe mux 12,50,100                          # OpenRouter
build\windows-release\bin\curl_bench.exe mux 12,50,100 --llm_provider anthropic
```

“同时在途”是这样量的：每个请求在发出前记下开始时间，答案回来后记下结束时间，把所有开始和结束排在一起数，数出最大的重叠。如果请求在一条连接里是一个一个排队的，这个数会小于 N。

<!-- src: bench/curl_bench.cpp#L512-L530 -->
```cpp
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
```

<!-- src: bench/curl_bench.cpp#L548-L553 -->
```cpp
  std::sort(events.begin(), events.end());  // at the same time an end (-1) comes before a start (+1)
  int now = 0;
  for (const auto& event : events) {
    now += event.second;
    run.max_overlap = (std::max)(run.max_overlap, static_cast<std::size_t>(now));
  }
```

OpenRouter（毫秒；`conns` 是新建的连接数，`overlap` 是同时在途的最大数量，`p95` 是第 95 百分位的单个请求耗时）：

| N | 版本 | 总耗时 | conns | overlap | p50 | p95 |
|---|---|---|---|---|---|---|
| 12 | HTTP/1.1 | 740 | 12 | 12 | 449 | 740 |
| 12 | HTTP/2 | 923 | 1 | 12 | 668 | 923 |
| 50 | HTTP/1.1 | 1126 | 50 | 50 | 552 | 920 |
| 50 | HTTP/2 | 1707 | 1 | 50 | 804 | 1138 |
| 100 | HTTP/1.1 | 1368 | 100 | 100 | 644 | 1144 |
| 100 | HTTP/2 | 3602 | 1 | 100 | 839 | 3234 |

N 为 100 又单独重复了 5 次：HTTP/1.1 是 1343 毫秒（p95 1034），HTTP/2 是 3093 毫秒（p95 2853）。

Anthropic：

| N | 版本 | 总耗时 | conns | overlap | p50 | p95 |
|---|---|---|---|---|---|---|
| 12 | HTTP/1.1 | 1200 | 12 | 12 | 555 | 1200 |
| 12 | HTTP/2 | 1146 | 1 | 12 | 624 | 1146 |
| 50 | HTTP/1.1 | 1296 | 50 | 50 | 603 | 1235 |
| 50 | HTTP/2 | 1239 | 1 | 50 | 542 | 1216 |
| 100 | HTTP/1.1 | 1799 | 100 | 100 | 756 | 1395 |
| 100 | HTTP/2 | 1643 | 1 | 100 | 655 | 1247 |

再试一次 N 为 300，各只跑 1 次。300 超过了 libcurl 默认的每条连接 100 个流，也超过了服务端允许的 256 个。

| 服务 | 版本 | 总耗时 | conns | overlap | 429 |
|---|---|---|---|---|---|
| OpenRouter | HTTP/1.1 | 11688 | 300 | 300 | 0 |
| OpenRouter | HTTP/2 | 11263 | 3 | 300 | 0 |
| Anthropic | HTTP/1.1 | 2581 | 300 | 300 | 0 |
| Anthropic | HTTP/2 | 1610 | 3 | 300 | 0 |

怎么读：

- **多路复用确实在工作：** 两家服务在 HTTP/2 下，12、50、100 个请求都只用 1 条连接，而且 `overlap` 等于 N，也就是 100 个请求同时都在等回答，并没有在连接里排队。HTTP/1.1 下每个请求一条连接，连接数等于 N。
- **超过每条连接的流数，libcurl 会再开连接：** N 为 300 时 HTTP/2 用了 3 条连接，没有请求被拒绝，也没有失败。这里起作用的是 libcurl 自己的限制 `CURLMOPT_MAX_CONCURRENT_STREAMS`，默认 100，300 个请求正好是 3 条。临时把它改成 256 再跑（Anthropic，1 次），变成 2 条连接（256 加 44）。服务端在 SETTINGS 里允许 256，所以实际的上限是两者中小的那一个。
- **Anthropic：** HTTP/2 在所有 N 上都不比 HTTP/1.1 慢，N 为 300 时快得多（1610 对 2581 毫秒），因为 HTTP/1.1 要开 300 条连接，每条都要握手。
- **OpenRouter：** N 为 100 时，一条 HTTP/2 连接上的 100 个请求总耗时是 100 条 HTTP/1.1 连接的两倍多（3093 对 1343 毫秒，5 次取中），p50 只差一点（738 对 643 毫秒），差在尾部：p95 是 2853 对 1034 毫秒。就是说多数请求一样快，但有一些请求在这一条连接上被拖慢了。原因我没有查：可能在网关，也可能在上游，这次没有证据说明。N 为 300 时两个版本都要 11 秒左右，这是服务本身在高并发下慢，和协议无关。
- **没有遇到 429。** 两家服务在 300 个同时在途的请求下都没有拒绝，当然这只说明这次、这个账号的限额没有碰到，不代表它们没有限流。

对机器人的含义：一个机器人同时在途的请求一般是几个到几十个，在这个范围里多路复用在两家服务上都正常，Anthropic 上还省连接；OpenRouter 上要是发现一条连接上的尾部延迟变大，可以把 `Options::version` 设成 `Version::kHttp1_1` 退回多条连接。

## 6. 验证

- `ctest`：144 个测试通过，本章新增 32 个：
  - `test_http_modes`（9 个）：响应带着各阶段耗时且前一个节点不晚于后一个节点；`Post` 每次新建连接，`Session` 第二次请求复用连接；`Session` 在一个请求失败后还能继续用；`PostMany` 的答案按请求顺序返回、同时进行的数量不超过上限、一个请求失败或超时不影响其他；6 个线程共用一个 `Share` 各自拿到自己的回答；对明文 http 要求 HTTP/2 时仍然是 HTTP/1.1。
  - `test_async_http`（23 个）：三个客户端各 6 个共同的测试（答案、错误状态也是答案、20 个请求同时在途各拿各的、一个请求失败不影响其他、超时只影响那一个、协程回到自己的线程）；A 和 B 各 2 个取消的测试（取消协程取消请求、销毁客户端时在途的请求以错误结束）；C 有一个测试说明取消要等当前请求结束。
- `scripts/check-build.ps1`：通过（编译 24 条、链接 15 条）。
- 崩溃的检查：基准程序里每个策略在 Debug 构建里各跑 30 到 60 次。修好销毁顺序之后，不共享连接缓存的 `Share` 连续 60 次没有崩溃；曾经出现的崩溃见第 8 节。
- 稳定性：`test_async_http` 连续运行 3 次，每次 23 个全部通过。
- 真实服务：B 在 TLS（Windows 的 Schannel）和 HTTP/2 上，12 个请求 12 个成功，只开一条连接。

### 对比

| | C 线程池加 Session | A 一个 curl 线程 | B `socket_action` |
|---|---|---|---|
| 线程（含事件循环线程） | 池线程数加 1 | 2 | 1 |
| 同时在途的请求 | 不超过线程数 | 不限 | 不限 |
| 20 个请求、各 300 毫秒（本机） | 1552 毫秒（4 线程） | 310 毫秒 | 311 毫秒 |
| 12 个问题（Anthropic，HTTP/2） | 2449 毫秒 | 1170 毫秒 | 1166 毫秒 |
| HTTP/2 多路复用 | 用不上（一线程一条连接） | 12 个请求 1 条连接 | 12 个请求 1 条连接 |
| 取消在途的请求 | 不能，要等它结束 | 能 | 能 |
| 销毁时在途的请求 | 等它们结束 | 以错误结束 | 以错误结束 |
| 从别的线程调用 | 可以 | 可以 | 不可以，只在事件循环线程上 |
| 代码量（`async_http.cpp` 里的行数） | 约 47 行 | 约 154 行 | 约 207 行 |
| 平台相关的坑 | 没有 | 没有 | 有：Windows 上要额外等 `wait_error` |

## 7. 技术资料

**结论：机器人默认用 A（`CurlThreadClient`）。**

- 对比 C：同样 12 个请求，C 受线程数限制，4 个线程要 3 轮，是 A 的两倍时间；要追上 A 就得开 12 个线程。A 只多一个线程，请求数不受限。更重要的是 C 不能取消一个正在进行的请求：Discord 里用户发 `/cancel` 或者要“停”，请求还要在池线程里一直等到它自己结束或超时，那个线程在这段时间里什么也干不了。
- 对比 B：A 和 B 在所有测量里一样快。B 少一个线程，但代码多约三分之一，而且有平台相关的坑（Windows 上要多等一个事件），必须只在事件循环线程上用。A 的线程安全靠 libcurl 明确保证可以跨线程调用的 `curl_multi_wakeup`，坑少。省下的一个线程不值得。
- 保留 B：如果以后想要进程里一个额外线程都没有，B 是可用的，测试已经覆盖了它。保留 C：最简单，工具里偶尔发一个请求、不需要取消的地方可以直接用。
- 这个结论会用来重写主线 1.1：`Post` 变成返回 `awaitable<Response>` 的函数，背后用 A；下一章 llm-1.3 把它抽成 `Transport` 接口。

**其他的取舍：**

- 为什么不共享连接缓存：见第 8 节，`Share` 只共享 DNS 缓存和 TLS 会话。在本章的数据里 `Share` 也没有带来可见的差别。
- 为什么 HTTP/2 不是选择的理由：几个到十几个请求时它对总耗时没有影响（回答时间远大于建立连接的时间），影响的是连接数（12 变 1），而 A 和 B 本来就能享受到。请求更多时两家服务表现不同（5.6）：Anthropic 上 HTTP/2 不慢，300 个并发时明显更快；OpenRouter 上 100 个并发时一条连接的尾部变慢。所以默认让 libcurl 自己选（`Version::kAny`，服务支持就用 HTTP/2），发现某个服务有问题时再退回 HTTP/1.1。
- libcurl 8.23.0，vcpkg 的 `curl[http2]` 带上 nghttp2 1.71.0，Windows 上 TLS 用 Schannel。HTTP/2 版本代码在 libcurl 里是 `CURL_HTTP_VERSION_2_0 = 3`，不是 2。
- 被否决的：libcurl 的 `curl_multi_socket_action` 配 libuv（没有协程支持，也不是我们的事件循环）；单独为每个请求开线程（线程数跟着请求数涨）；`Share` 加 `CURL_LOCK_DATA_CONNECT`（崩溃）。
- 限流（429）没有测，也就没有测“12 个同时发会不会被服务限流”：本章的 12 个请求在两家服务上都没有遇到限流。线程池或 multi 的并发数要不要设上限，留给 llm-1.8（重试、限流与降级）。

## 8. 常见问题与坑

**段错误：缓冲区比句柄先销毁。** 这是遇到的第一个崩溃。`Transfer` 里的请求头列表、请求体和错误缓冲区，要比 libcurl 句柄活得久：句柄在清理时还会访问它的选项指向的缓冲区。`Session::Impl` 和每个 job 里，`transfer` 都写在 `easy` 前面，成员按声明的反序销毁，句柄先销毁。声明顺序反了，大约每 4 到 5 次运行崩一次，而且每次崩的位置不一样。

**`Share` 的锁也要比 share 句柄晚销毁。** `curl_share_cleanup` 会调用解锁回调，如果这时锁已经销毁，同样崩溃。

**共享连接缓存会在 libcurl 里崩溃。** 修好上面两个问题之后，只有共享了 `CURL_LOCK_DATA_CONNECT` 的那种策略还偶尔崩：Debug 构建大约每 5 次运行崩一次，Release 构建 60 次里崩过 1 次，位置在 libcurl 的 `url_match_destination`。换成递归锁没有用。不共享连接缓存，连续 60 次没有崩。我没有去查是 libcurl 的问题还是我们用法的问题，选择绕开：想复用连接的线程，自己保留一个 `Session`。

**明文 http 上设 `CURLOPT_PIPEWAIT` 会把请求串起来。** 设了它，后面的请求会等第一个请求的连接；HTTPS 下握手之后就知道服务端支不支持 HTTP/2，等待是值得的，明文 http 没有握手，libcurl 要等第一个答案回来才知道，等待只会让请求排队：8 个各 200 毫秒的请求用了 421 毫秒，而不是 218 毫秒。所以 `PIPEWAIT` 只在 HTTPS 且没有强制 HTTP/1.1 时设（`WaitForMultiplexing`）。

**Windows 上连接失败不是“可写”。** B 里监视套接字时，连接被拒绝或不可达，在 Windows 上出现在 select 的异常集合里，而不是可写。不额外 `async_wait(wait_error)`，失败要等 libcurl 自己的连接超时。症状是：一个指向没人监听的端口的请求，要等 10 秒才失败，其他请求都正常。

**`release()` 套接字，不要让 Asio 关闭它。** B 里套接字归 libcurl 所有。libcurl 说“不用监视了”（`CURL_POLL_REMOVE`）时，要对 Asio 的 `tcp::socket` 调用 `release()`，不能直接销毁它，否则同一个套接字会被关两次。

**在 libcurl 的回调里不能再调用 libcurl。** `OnTimer` 要求“立刻处理”（0 毫秒）时，要把 `curl_multi_socket_action` `post` 出去，不能在回调里直接调。`Done` 里恢复协程也用 `post` 而不是 `dispatch`，原因同上。

**协程恢复要回到协程自己的执行器。** A 的结果是在 curl 线程里得到的，不能在那里直接恢复协程，否则协程会在 curl 线程上运行。`Suspend` 里把恢复的动作投回协程所在的执行器。

**`std::min` 在 Windows 上要加括号。** `curl/curl.h` 会间接包含 `windows.h`，它定义了 `min` 和 `max` 宏，`std::min(a, b)` 要写成 `(std::min)(a, b)`。

**真实服务的数字有波动。** 同一个问题，同一个服务，单个请求从 170 毫秒到 1 秒多都出现过，总耗时又取决于最慢的那个回答。比较两种策略要重复几次取居中的一次，两次不够，本章用了 5 次。差别小于一成的，不要当成结论。

**基准要用 Release。** Debug 构建没有优化，数字不能拿来比较；Debug 只用来跑测试和查崩溃。

**HTTP/2 不是 HTTP/1.1 的直接升级。** 同一条 TCP 连接丢一个包，上面所有的流都会等；HTTP/3 用 QUIC 解决了这个问题，三家服务里 OpenAI 和 OpenRouter 已经宣告了它，Anthropic 没有。

## 9. 练习

1. 把 `PoolClient` 的线程数加到 12，在 Anthropic 上重测，它能追上 A 吗？代价是什么（线程数、连接数）？
2. 给 `CurlThreadClient` 加一个同时在途请求数的上限（超过的排队），用 `CURLMOPT_MAX_HOST_CONNECTIONS` 和 `CURLMOPT_MAX_CONCURRENT_STREAMS` 对比，看哪种更适合限制对同一个服务的并发。
3. 把 `Share` 加上 `CURL_LOCK_DATA_CONNECT` 之后，用 `curl_bench local 20 50 4 1` 连续运行几十次，复现崩溃。再试着只共享 DNS，只共享 TLS 会话，看哪种有可见的收益。
4. 在 `SocketClient` 里去掉 `wait_error`，写一个测试复现“连接被拒绝要等 10 秒”。
5. `idle` 只测到 90 秒。把间隔加到 5 分钟、15 分钟，看哪一家先关连接，再用 `Session` 发请求，看 libcurl 是不是悄悄重试了一条新的。
6. OpenRouter 在 100 个并发时一条 HTTP/2 连接的尾部变慢，把 `CURLOPT_VERBOSE` 的输出和每个流的收发时间记下来，看慢的流是在发出请求之前就在等，还是在等回答。试着 N 取 60、80、120，看变慢从哪里开始。
7. 给三个客户端加一个“同时在途的请求数”的统计（原子计数器），在 20 个请求的本机基准里确认 C 同时在途不超过 4，A 和 B 是 20。

## 10. 支线

本章节没有可以开启的支线。

## 11. 下一章预告

下一章 llm-1.3 把 `Post` 抽成 `Transport` 接口：签名是 `asio::awaitable<Response> Post(PostRequest, Options)`，时钟可以注入，本章的三个客户端都成为它的实现，再写一套契约测试跑遍所有实现。接口里还要定下取消如何传到在途的请求、传输层的错误怎么分类（可重试、不可重试，供重试使用），以及日志里怎么对 `Authorization` 脱敏。
