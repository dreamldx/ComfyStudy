#pragma once

#include <boost/asio.hpp>

#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "gateway.h"

// A Gateway for tests: the test hands it messages and commands, and it writes down what the bot sent.
class FakeGateway final : public bot::Gateway {
 public:
  struct Sent {
    std::string channel;
    std::string text;
    std::string reply_to;
  };
  struct Replied {
    bot::Command command;
    std::string text;
  };

  void OnMessage(std::function<void(bot::IncomingMessage)> handler) override { on_message_ = std::move(handler); }
  void OnCommand(std::function<void(bot::Command)> handler) override { on_command_ = std::move(handler); }
  void Start() override { started = true; }
  boost::asio::awaitable<void> Stop() override {
    stopped = true;
    co_return;
  }

  boost::asio::awaitable<std::string> Send(const std::string& channel_id, const std::string& text, const std::string& reply_to) override {
    if (fail_sends > 0) {
      --fail_sends;
      throw std::runtime_error("the platform refused the message");
    }
    sent.push_back({channel_id, text, reply_to});
    co_return "sent-" + std::to_string(sent.size());
  }
  boost::asio::awaitable<void> Typing(const std::string& channel_id) override {
    ++typing[channel_id];
    co_return;
  }
  boost::asio::awaitable<void> Reply(const bot::Command& command, const std::string& text) override {
    replies.push_back({command, text});
    co_return;
  }

  // What the platform would do: tell the bot.
  void Deliver(bot::IncomingMessage message) { on_message_(std::move(message)); }
  void Use(bot::Command command) { on_command_(std::move(command)); }

  std::vector<Sent> sent;
  std::vector<Replied> replies;
  std::map<std::string, int> typing;
  bool started = false;
  bool stopped = false;
  int fail_sends = 0;  // the next this many Send calls throw

 private:
  std::function<void(bot::IncomingMessage)> on_message_;
  std::function<void(bot::Command)> on_command_;
};
