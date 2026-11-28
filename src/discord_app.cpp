#include "discord_app.h"

#include <spdlog/spdlog.h>

#include <atomic>
#include <csignal>
#include <iostream>
#include <memory>

#include "agent.h"
#include "bot.h"
#include "chat.h"
#include "context.h"
#include "curl_transport.h"
#include "dpp_gateway.h"
#include "history.h"
#include "llm_client.h"
#include "skill_setup.h"
#include "tool_runtime.h"

#if defined(_WIN32)
#include <windows.h>
#endif

namespace bot {

namespace {

#if defined(_WIN32)
// Closing the console window ends the process a few seconds after the handler returns: ask the bot to stop and
// give it that time to finish. (Ctrl+C is handled by the signal_set below.)
boost::asio::io_context* g_io = nullptr;
Bot* g_bot = nullptr;
std::atomic<bool> g_finished{false};

BOOL WINAPI OnConsoleEvent(DWORD type) {
  if (type != CTRL_CLOSE_EVENT && type != CTRL_LOGOFF_EVENT && type != CTRL_SHUTDOWN_EVENT) return FALSE;
  if (g_io != nullptr && g_bot != nullptr) boost::asio::post(*g_io, [] { g_bot->RequestStop(); });
  for (int i = 0; i < 45 && !g_finished; ++i) Sleep(100);
  return TRUE;
}
#endif

}  // namespace

int RunDiscord(const options::Options& options) {
  try {
    const config::Values& values = options.values;
    const auto token = values.find("discord_token");
    if (token == values.end() || token->second.empty()) {
      std::cerr << "error: the setting discord_token is not set: put the token of the bot in the config file "
                   "(discord_token = ...), in the environment variable DISCORD_TOKEN or on the command line\n";
      return 1;
    }
    const std::string provider_name = llm::DefaultProviderName(values);
    const llm::Provider provider = llm::MakeProvider(values, provider_name);  // throws and names the missing setting

    BotOptions settings;
    settings.system = options.system;
    settings.llm = llm::OptionsFromSettings(values);
    settings.limits = agent::LimitsFromSettings(values);
    settings.tools = agent::RuntimeOptionsFromSettings(values);
    settings.budget_tokens = agent::ContextBudgetFromSettings(values);

    std::shared_ptr<agent::NativeTools> library;
    agent::SkillCatalog catalog;
    if (options.tools) {
      const agent::SkillSetup setup = agent::LoadSkillSetup(values);
      library = setup.library;
      catalog = setup.catalog;
    }

    llm::UseUtf8Console();
    const auto transport = http::MakeCurlTransport();
    const auto history = std::make_shared<agent::MemoryHistory>();

    boost::asio::io_context io;
    DppGateway gateway(token->second, io.get_executor());
    Bot bot(gateway, *transport, provider, history, settings, library, catalog);

    boost::asio::signal_set signals(io, SIGINT, SIGTERM);
    signals.async_wait([&bot](const boost::system::error_code& error, int) {
      if (!error) bot.RequestStop();
    });
#if defined(_WIN32)
    g_io = &io;
    g_bot = &bot;
    SetConsoleCtrlHandler(OnConsoleEvent, TRUE);
#endif

    spdlog::info("the bot is starting (provider {}, model {})", provider.name, provider.model);
    std::exception_ptr failure;
    boost::asio::co_spawn(io, bot.Run(), [&](std::exception_ptr error) {
      failure = error;
      signals.cancel();  // or the io_context would wait for a signal for ever
    });
    io.run();

#if defined(_WIN32)
    SetConsoleCtrlHandler(OnConsoleEvent, FALSE);
    g_finished = true;
    g_io = nullptr;
    g_bot = nullptr;
#endif
    if (failure) std::rethrow_exception(failure);
    if (bot.Failure()) {
      std::cerr << "error: " << *bot.Failure() << '\n';
      return 1;
    }
    spdlog::info("the bot has stopped");
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << '\n';
    return 1;
  }
}

}  // namespace bot
