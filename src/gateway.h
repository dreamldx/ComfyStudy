#pragma once

#include <boost/asio.hpp>

#include <functional>
#include <string>

// The seam between the bot and a chat platform (chapter 1.7). The bot (bot.h) only knows this interface;
// Discord (dpp_gateway.h) is one implementation of it, a fake in the tests is another. So the lanes, the
// queue, the splitting of long answers and the commands can be tested without a connection.
namespace bot {

// A message that someone wrote in a place the bot can see.
struct IncomingMessage {
  std::string id;          // unique for a message; the same id again is the same message (a reconnect sends it twice)
  std::string channel_id;  // the conversation it belongs to: a channel, a direct message, or a thread
  std::string author_id;
  std::string author_name;
  bool author_is_bot = false;  // the bot itself, or another bot
  bool mentions_bot = false;   // it names the bot
  bool is_dm = false;          // a direct message
  bool is_thread = false;      // written in a thread
  std::string content;         // the text, with the mention of the bot taken out
};

// A slash command, with what is needed to answer it.
struct Command {
  std::string name;  // "status" or "stop"
  std::string channel_id;
  std::string user_id;
  std::string handle;  // what the platform needs to answer this command (the interaction); the bot does not look at it
};

// Everything is called on the thread of the io_context the bot runs on; an implementation that gets its events
// on other threads hands them over (post) before it calls the functions it was given.
class Gateway {
 public:
  virtual ~Gateway() = default;

  // Who is told about messages and commands. Set before Start.
  virtual void OnMessage(std::function<void(IncomingMessage)> handler) = 0;
  virtual void OnCommand(std::function<void(Command)> handler) = 0;

  virtual void Start() = 0;                         // connect and begin delivering
  virtual boost::asio::awaitable<void> Stop() = 0;  // stop delivering, disconnect

  // Writes a message in the channel and gives back its id. `reply_to` (may be empty) is the id of the message it
  // answers. A message never notifies anyone by name: whatever the text says (@everyone, a user), nobody is
  // pinged. Throws when the message could not be sent.
  virtual boost::asio::awaitable<std::string> Send(const std::string& channel_id, const std::string& text,
                                                   const std::string& reply_to) = 0;

  // "The bot is typing...", which the platform shows for a few seconds.
  virtual boost::asio::awaitable<void> Typing(const std::string& channel_id) = 0;

  // The answer to a command, shown to the person who used it only.
  virtual boost::asio::awaitable<void> Reply(const Command& command, const std::string& text) = 0;
};

}  // namespace bot
