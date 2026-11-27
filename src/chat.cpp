#include "chat.h"

#include <boost/asio.hpp>

#include <csignal>
#include <iostream>
#include <memory>

#include "console_hook.h"

#if defined(_WIN32)
#include <windows.h>
#endif

namespace llm {
namespace {

using agent::Trimmed;

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
             const Options& options, const std::string& system, agent::ToolSet* tools, const agent::TurnLimits& limits,
             std::istream& in, std::ostream& out, std::ostream& err, const agent::Hooks& extra_hooks,
             const ContextSettings& context) {
  const Provider* current = Find(providers, start_name);
  if (current == nullptr) {
    throw Error("there is no provider called \"" + start_name + "\"; available: " + Names(providers));
  }

  const std::shared_ptr<agent::History> history = context.history ? context.history : std::make_shared<agent::MemoryHistory>();
  Conversation conversation;
  conversation.system = system;
  conversation.messages = history->Load(context.channel);
  boost::asio::io_context io;
  agent::Hooks hooks = {std::make_shared<agent::ConsoleHook>(out, err, limits)};
  hooks.insert(hooks.end(), extra_hooks.begin(), extra_hooks.end());

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
        history->Clear(context.channel);
        conversation.messages.clear();
        if (tools != nullptr) tools->StartOver();
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

    // One Turn. The user's message goes in; whatever happens, a Turn that does not end with an answer
    // is taken out again by RunTurn, so the history only holds questions that were answered.
    conversation.messages = history->Load(context.channel);
    const std::size_t before = conversation.messages.size();
    conversation.messages.push_back({Role::kUser, {TextBlock{line}}});
    agent::ContextPlan plan;
    plan.system = [&] {
      std::vector<agent::Section> sections = {{"", system}};
      sections.insert(sections.end(), context.system_extra.begin(), context.system_extra.end());
      return sections;
    };
    plan.state = context.state;
    plan.budget_tokens = context.budget_tokens > 0 ? context.budget_tokens : agent::DefaultBudget(*current, options);
    plan.count = context.count;
    try {
      agent::TurnControl control;
      boost::asio::signal_set signals(io, SIGINT);  // Ctrl+C stops this Turn, only while it runs
      signals.async_wait([&control](const boost::system::error_code& error, int /*signal*/) {
        if (!error) control.Cancel();
      });
      io.restart();
      auto outcome = boost::asio::co_spawn(
          io,
          [&]() -> boost::asio::awaitable<agent::TurnOutcome> {
            const agent::TurnOutcome result =
                co_await agent::RunTurn(transport, *current, conversation, options, tools, hooks, limits, nullptr, &plan);
            signals.cancel();  // or the io_context would wait for Ctrl+C for ever
            co_return result;
          },
          boost::asio::bind_cancellation_slot(control.slot(), boost::asio::use_future));
      io.run();
      // the hooks have said everything there is to say; only a Turn that ended with an answer is kept
      if (outcome.get().reason == agent::TurnOutcome::Reason::kDone) {
        history->Append(context.channel, std::vector<Message>(conversation.messages.begin() + static_cast<std::ptrdiff_t>(before), conversation.messages.end()));
      }
    } catch (const std::exception& e) {
      err << "error: " << e.what() << '\n';
      conversation.messages.erase(conversation.messages.begin() + static_cast<std::ptrdiff_t>(before), conversation.messages.end());
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
