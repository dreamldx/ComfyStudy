#pragma once

#include <httplib.h>

#include <string>
#include <thread>

// A small HTTP server on the loopback interface for tests. Register the handlers on server(),
// then call Start(); it listens on a free port until the object is destroyed.
class TestServer {
 public:
  httplib::Server& server() { return server_; }

  void Start() {
    port_ = server_.bind_to_any_port("127.0.0.1");
    thread_ = std::thread([this] { server_.listen_after_bind(); });
    server_.wait_until_ready();
  }

  ~TestServer() {
    server_.stop();
    if (thread_.joinable()) thread_.join();
  }

  int port() const { return port_; }
  std::string Url(const std::string& path = "") const {
    return "http://127.0.0.1:" + std::to_string(port_) + path;
  }

 private:
  httplib::Server server_;
  std::thread thread_;
  int port_ = 0;
};
