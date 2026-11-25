#include "chat.h"

#include <boost/asio.hpp>

#include <iostream>
#include <sstream>

#include <spdlog/spdlog.h>

#include "llm_client.h"

#if defined(_WIN32)
#include <windows.h>
#endif

namespace llm {
namespace {

std::string Trimmed(const std::string& text) {
  const char* const spaces = " \t\r\n";
  const std::size_t first = text.find_first_not_of(spaces);
  if (first == std::string::npos) return "";
  return text.substr(first, text.find_last_not_of(spaces) - first + 1);
}

std::string Names(const std::vector<Provider>& providers) {
  std::string names;
  for (const Provider& provider : providers) names += (names.empty() ? "" : ", ") + provider.name;
  return names;
}

const Provider* Find(const std::vector<Provider>& providers, const std::string& name) {
  for (const Provider& provider : providers) {
    if (provider.name == name) return &provider;
  }
  return nullptr;
}

}  // namespace

void RunChat(http::Transport& transport, const std::vector<Provider>& providers, const std::string& start_name,
             const Options& options, const std::string& system, std::istream& in, std::ostream& out,
             std::ostream& err) {
  const Provider* current = Find(providers, start_name);
  if (current == nullptr) {
    throw Error("there is no provider called \"" + start_name + "\"; available: " + Names(providers));
  }

  Conversation conversation;
  conversation.system = system;
  boost::asio::io_context io;

  std::string line;
  while (out << "> " << std::flush, std::getline(in, line)) {
    line = Trimmed(line);
    if (line.empty()) continue;

    if (line.front() == '/') {
      const std::size_t space = line.find(' ');
      const std::string command = line.substr(0, space);
      const std::string argument = space == std::string::npos ? "" : Trimmed(line.substr(space + 1));
      if (command == "/exit") break;
      if (command == "/reset") {
        conversation.messages.clear();
        out << "history cleared\n";
      } else if (command == "/help") {
        out << "/use <name>  switch to another provider (" << Names(providers) << ")\n"
            << "/reset       forget the history\n"
            << "/exit        end the chat\n"
            << "/help        show this list\n";
      } else if (command == "/use") {
        const Provider* chosen = argument.empty() ? nullptr : Find(providers, argument);
        if (chosen == nullptr) {
          err << (argument.empty() ? "usage: /use <name>" : "provider \"" + argument + "\" is not available")
              << "; available: " << Names(providers) << '\n';
        } else {
          current = chosen;
          out << "now using " << current->name << '\n';
          const std::size_t foreign = CountForeignReasoning(conversation, current->protocol);
          if (foreign > 0) {
            out << foreign << " reasoning block(s) in the history come from another protocol and will not be sent\n";
          }
        }
      } else {
        err << "unknown command \"" << command << "\"; /help lists the commands\n";
      }
      continue;
    }

    conversation.messages.push_back({Role::kUser, {TextBlock{line}}});
    try {
      io.restart();
      auto answer = boost::asio::co_spawn(io, Generate(transport, *current, conversation, options),
                                          boost::asio::use_future);
      io.run();
      const Reply reply = answer.get();  // throws what Generate threw
      const std::string text = Trimmed(reply.Text());  // some services start the text with blank lines
      if (text.empty()) {
        err << "the model returned no text (stopped because: " << reply.stop_reason << ")\n";
        conversation.messages.pop_back();
        continue;
      }
      out << text << '\n';  // the history below keeps the text as it came
      spdlog::info("stopped because: {}; tokens in/out: {}/{}", reply.stop_reason, reply.input_tokens.value_or(-1),
                   reply.output_tokens.value_or(-1));
      conversation.messages.push_back(reply.message);
    } catch (const std::exception& e) {
      err << "error: " << e.what() << '\n';
      conversation.messages.pop_back();
    }
  }
}

void UseUtf8Console() {
#if defined(_WIN32)
  DWORD mode = 0;
  if (GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode)) SetConsoleCP(CP_UTF8);
  if (GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &mode)) SetConsoleOutputCP(CP_UTF8);
#endif
}

}  // namespace llm
