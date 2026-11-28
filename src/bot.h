#pragma once

#include <boost/asio.hpp>

#include <chrono>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>

#include "agent.h"
#include "context.h"
#include "gateway.h"
#include "history.h"
#include "native_tools.h"
#include "provider.h"
#include "skill.h"
#include "tool_runtime.h"
#include "transport.h"

// The Discord bot (chapter 1.7), on top of a Gateway (gateway.h): who may talk, in what order the messages of
// one channel are handled, what is sent back, and how it stops. It runs on one io_context thread and nothing in
// it is locked: the gateway hands every event over to that thread.
namespace bot {

struct BotOptions {
  std::string system;                  // the operator's instruction for the model (--system); may be empty
  llm::Options llm;                    // reasoning, answer size, prompt cache
  agent::TurnLimits limits;            // steps and the deadline of one answer
  agent::RuntimeOptions tools;         // time and size limits of the tool runtime of each channel
  std::size_t budget_tokens = 0;       // 0: what the window of the provider leaves
  agent::TokenCounter count;           // empty: EstimateTokens
  std::chrono::milliseconds typing_interval{8000};  // how often "typing..." is shown again while an answer is being made
  std::chrono::milliseconds shutdown_wait{5000};    // how long a stop waits for the answers that are being made
  std::function<std::string()> make_error_id;       // empty: MakeErrorId
  std::function<std::chrono::system_clock::time_point()> now;  // empty: the system clock (the date in the state)
  std::optional<std::chrono::minutes> utc_offset;   // empty: the local time zone
};

// The system prompt section about how Discord shows text: which Markdown works, which does not, the limit of a
// message, and that mentions notify nobody.
agent::Section DiscordFormatSection();

// "@everyone", "@here" and "<@...>" with a zero-width space inserted, so that they stay text. The gateway also
// sends every message with mentions switched off; this is a second lock on the same door.
std::string NeutralizeMentions(const std::string& text);

class Bot {
 public:
  // `library` and `catalog` (may be empty) are the tools that Skills can bring: each channel gets its own runtime
  // and its own loaded Skills.
  Bot(Gateway& gateway, http::Transport& transport, llm::Provider provider, std::shared_ptr<agent::History> history,
      BotOptions options, std::shared_ptr<agent::NativeTools> library = nullptr, agent::SkillCatalog catalog = {});
  ~Bot();
  Bot(const Bot&) = delete;
  Bot& operator=(const Bot&) = delete;

  // Starts the gateway and runs until RequestStop: then stops taking messages, cancels the answers that are being
  // made, waits for them (at most `shutdown_wait`) and stops the gateway.
  boost::asio::awaitable<void> Run();

  // Asks Run to finish. Call it on the io_context thread (post from a signal handler or another thread).
  void RequestStop();

  std::size_t LaneCount() const;  // how many channels have a lane now (for tests and for /status)

 private:
  struct Lane;
  struct TypingState;

  void Accept(IncomingMessage message);
  bool Remember(const std::string& message_id);  // false when the id was seen already
  Lane& LaneFor(const std::string& channel_id);
  boost::asio::awaitable<void> RunLane(std::string channel_id);
  boost::asio::awaitable<void> RunJob(Lane& lane, std::string text, std::string reply_to);
  boost::asio::awaitable<void> TypingLoop(std::string channel_id, std::shared_ptr<TypingState> state);
  boost::asio::awaitable<void> SendAll(const std::string& channel_id, const std::string& text, const std::string& reply_to);
  boost::asio::awaitable<void> HandleCommand(Command command);
  std::string ErrorId();

  Gateway& gateway_;
  http::Transport& transport_;
  llm::Provider provider_;
  std::shared_ptr<agent::History> history_;
  BotOptions options_;
  std::shared_ptr<agent::NativeTools> library_;
  agent::SkillCatalog catalog_;

  boost::asio::any_io_executor executor_;
  std::unique_ptr<boost::asio::steady_timer> stop_timer_;
  bool stop_requested_ = false;
  bool stopping_ = false;
  std::map<std::string, std::unique_ptr<Lane>> lanes_;
  int active_jobs_ = 0;  // lanes with an answer being made
  int job_counter_ = 0;
  std::unordered_set<std::string> seen_;
  std::deque<std::string> seen_order_;
};

}  // namespace bot
