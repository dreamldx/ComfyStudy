#pragma once

#include <boost/asio.hpp>

#include <memory>
#include <string>

#include "gateway.h"

namespace bot {

// The Gateway for Discord, on DPP. DPP has an event loop and a thread pool of its own; its callbacks run on those
// threads. Each one does the least it can (turn the event into our types, skip what the bot must not see) and
// posts the rest to `executor`, the io_context of the bot, so nothing else of the bot runs on a DPP thread.
// Answers from Discord (a message was sent, or refused) come back the same way.
//
// Every message is sent with mentions switched off, and a reply does not ping the person it answers.
// The slash commands /status and /stop are registered when the connection is ready; a command is answered with
// one private reply (the bot answers it at once, so there is no deferred answer to get out of order).
class DppGateway final : public Gateway {
 public:
  DppGateway(const std::string& token, boost::asio::any_io_executor executor);
  ~DppGateway() override;

  void OnMessage(std::function<void(IncomingMessage)> handler) override;
  void OnCommand(std::function<void(Command)> handler) override;
  void OnFatal(std::function<void(std::string)> handler) override;
  void Start() override;
  boost::asio::awaitable<void> Stop() override;
  boost::asio::awaitable<std::string> Send(const std::string& channel_id, const std::string& text, const std::string& reply_to) override;
  boost::asio::awaitable<void> Typing(const std::string& channel_id) override;
  boost::asio::awaitable<void> Reply(const Command& command, const std::string& text) override;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace bot
