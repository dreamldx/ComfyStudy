#include "dpp_gateway.h"

#include <dpp/dpp.h>
#include <spdlog/spdlog.h>

#include <atomic>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>

#include "dpp_convert.h"

namespace bot {

namespace asio = boost::asio;

namespace {

spdlog::level::level_enum LevelOf(dpp::loglevel severity) {
  switch (severity) {
    case dpp::ll_trace: return spdlog::level::trace;
    case dpp::ll_debug: return spdlog::level::debug;
    case dpp::ll_info: return spdlog::level::info;
    case dpp::ll_warning: return spdlog::level::warn;
    case dpp::ll_error: return spdlog::level::err;
    case dpp::ll_critical: return spdlog::level::critical;
  }
  return spdlog::level::info;
}

}  // namespace

struct DppGateway::Impl {
  Impl(const std::string& token, asio::any_io_executor ex)
      : cluster(token, dpp::i_default_intents | dpp::i_message_content), executor(std::move(ex)) {}

  dpp::cluster cluster;
  asio::any_io_executor executor;
  std::function<void(IncomingMessage)> on_message;
  std::function<void(Command)> on_command;
  std::function<void(std::string)> on_fatal;
  std::atomic<bool> fatal_told{false};
  std::atomic<bool> stopped{false};
  std::atomic<bool> commands_registered{false};

  // The slash commands that were taken and are waiting for their answer (the interaction needs to be kept).
  std::mutex mutex;
  std::unordered_map<std::string, dpp::slashcommand_t> pending;
  unsigned long long next_handle = 0;

  // Runs `start`, which sends one request to Discord with the callback it is given, and waits for the callback
  // (it comes on a DPP thread; the coroutine goes on on the executor). Gives back the id of the message that came
  // back, or throws what Discord refused with.
  template <class Start>
  asio::awaitable<std::string> Await(Start start) {
    asio::any_io_executor ex = executor;
    co_return co_await asio::async_initiate<decltype(asio::use_awaitable), void(std::exception_ptr, std::string)>(
        [ex, start = std::move(start)](auto handler) mutable {
          auto shared = std::make_shared<decltype(handler)>(std::move(handler));
          start([ex, shared](const dpp::confirmation_callback_t& callback) {
            std::exception_ptr error;
            std::string id;
            if (callback.is_error()) {
              error = std::make_exception_ptr(std::runtime_error("Discord refused the request: " + callback.get_error().message));
            } else if (std::holds_alternative<dpp::message>(callback.value)) {
              id = std::get<dpp::message>(callback.value).id.str();
            }
            asio::post(ex, [shared, error, id]() mutable { std::move (*shared)(error, id); });
          });
        },
        asio::use_awaitable);
  }
};

DppGateway::DppGateway(const std::string& token, asio::any_io_executor executor) : impl_(std::make_unique<Impl>(token, std::move(executor))) {}

DppGateway::~DppGateway() = default;

void DppGateway::OnMessage(std::function<void(IncomingMessage)> handler) { impl_->on_message = std::move(handler); }
void DppGateway::OnCommand(std::function<void(Command)> handler) { impl_->on_command = std::move(handler); }
void DppGateway::OnFatal(std::function<void(std::string)> handler) { impl_->on_fatal = std::move(handler); }

void DppGateway::Start() {
  Impl& s = *impl_;

  s.cluster.on_log([&s](const dpp::log_t& event) {
    spdlog::log(LevelOf(event.severity), "[dpp] {}", event.message);
    // DPP tries again and again after these; the user has to change something, so say what and stop.
    if (const auto problem = FatalGatewayProblem(event.message)) {
      if (!s.fatal_told.exchange(true)) {
        asio::post(s.executor, [&s, why = *problem] {
          if (s.on_fatal) s.on_fatal(why);
        });
      }
    }
  });

  s.cluster.on_ready([&s](const dpp::ready_t&) {
    if (s.commands_registered.exchange(true)) return;  // a reconnect says "ready" again
    try {
      s.cluster.global_bulk_command_create({dpp::slashcommand("status", "Show what the bot is doing in this channel", s.cluster.me.id),
                                            dpp::slashcommand("stop", "Stop the answer that is being made in this channel", s.cluster.me.id)});
    } catch (const std::exception& e) {
      spdlog::error("could not register the commands: {}", e.what());
    }
  });

  // This runs on a thread of DPP's pool: turn the event into ours and hand it over.
  s.cluster.on_message_create([&s](const dpp::message_create_t& event) {
    try {
      if (s.stopped) return;
      spdlog::debug("a message arrived on thread {} of DPP", std::hash<std::thread::id>{}(std::this_thread::get_id()));
      const dpp::channel* channel = dpp::find_channel(event.msg.channel_id);
      const bool in_thread = channel != nullptr && IsThreadType(channel->get_type());
      IncomingMessage message = ConvertMessage(event.msg, s.cluster.me.id, in_thread);
      if (message.author_id == s.cluster.me.id.str()) message.author_is_bot = true;  // never answer itself
      asio::post(s.executor, [&s, message = std::move(message)]() mutable {
        if (!s.stopped && s.on_message) s.on_message(std::move(message));
      });
    } catch (const std::exception& e) {
      spdlog::error("a message could not be read: {}", e.what());
    }
  });

  s.cluster.on_slashcommand([&s](const dpp::slashcommand_t& event) {
    try {
      if (s.stopped) return;
      Command command;
      command.name = event.command.get_command_name();
      command.channel_id = event.command.channel_id.str();
      command.user_id = event.command.usr.id.str();
      {
        const std::lock_guard<std::mutex> lock(s.mutex);
        command.handle = std::to_string(++s.next_handle);
        s.pending.emplace(command.handle, event);
      }
      asio::post(s.executor, [&s, command = std::move(command)]() mutable {
        if (!s.stopped && s.on_command) s.on_command(std::move(command));
      });
    } catch (const std::exception& e) {
      spdlog::error("a command could not be read: {}", e.what());
    }
  });

  s.cluster.start(dpp::st_return);  // DPP runs on threads of its own from here on
}

asio::awaitable<void> DppGateway::Stop() {
  impl_->stopped = true;
  impl_->cluster.shutdown();
  co_return;
}

asio::awaitable<std::string> DppGateway::Send(const std::string& channel_id, const std::string& text, const std::string& reply_to) {
  Impl& s = *impl_;
  dpp::message message(dpp::snowflake(channel_id), text);
  message.set_allowed_mentions(false, false, false, false, {}, {});  // nobody is notified, whatever the text says
  if (!reply_to.empty()) message.set_reference(dpp::snowflake(reply_to));
  co_return co_await s.Await([&s, message](auto callback) { s.cluster.message_create(message, callback); });
}

asio::awaitable<void> DppGateway::Typing(const std::string& channel_id) {
  impl_->cluster.channel_typing(dpp::snowflake(channel_id));  // nobody waits for it
  co_return;
}

asio::awaitable<void> DppGateway::Reply(const Command& command, const std::string& text) {
  Impl& s = *impl_;
  std::optional<dpp::slashcommand_t> event;
  {
    const std::lock_guard<std::mutex> lock(s.mutex);
    const auto found = s.pending.find(command.handle);
    if (found != s.pending.end()) {
      event = found->second;
      s.pending.erase(found);
    }
  }
  if (!event) co_return;
  dpp::message message(text);
  message.set_allowed_mentions(false, false, false, false, {}, {});
  message.set_flags(dpp::m_ephemeral);  // only the person who used the command sees it
  event->reply(message);                // the bot answers a command at once, within the 3 seconds Discord allows
  co_return;
}

}  // namespace bot
