#include "bot.h"

#include <spdlog/spdlog.h>

#include <algorithm>

#include "console_hook.h"  // Trimmed
#include "error_id.h"
#include "message_split.h"
#include "skill_tools.h"

namespace bot {

namespace asio = boost::asio;

namespace {

// What the user is told. In one place so that the wording is easy to change.
const char* const kStopped = "已停止。";
const char* const kShuttingDown = "机器人正在关闭，这一次已经停止。";

std::string TroubleMessage(agent::TurnOutcome::Reason reason, const std::string& id) {
  using Reason = agent::TurnOutcome::Reason;
  switch (reason) {
    case Reason::kTimedOut: return "这一次花的时间太长，已经停止（错误编号 " + id + "）。";
    case Reason::kMaxSteps: return "这一次用完了步数还没有得出答案（错误编号 " + id + "）。可以把问题拆小一点再问。";
    case Reason::kNoText: return "模型没有给出回答（错误编号 " + id + "）。请再问一次。";
    default: return "出错了（错误编号 " + id + "），请稍后再试。";
  }
}

const char* ReasonName(agent::TurnOutcome::Reason reason) {
  using Reason = agent::TurnOutcome::Reason;
  switch (reason) {
    case Reason::kDone: return "done";
    case Reason::kMaxSteps: return "max_steps";
    case Reason::kCancelled: return "cancelled";
    case Reason::kTimedOut: return "timed_out";
    case Reason::kFailed: return "failed";
    case Reason::kNoText: return "no_text";
  }
  return "?";
}

// Counts the Steps of the Turn, for /status.
class StepHook final : public agent::AgentHook {
 public:
  explicit StepHook(int* steps) : steps_(steps) {}
  asio::awaitable<void> OnStepStarted(const agent::StepStarted& event) override {
    *steps_ = event.step;
    co_return;
  }

 private:
  int* steps_;
};

// The text of the last message of the conversation (the answer of the model).
std::string LastText(const llm::Conversation& conversation) {
  if (conversation.messages.empty()) return "";
  std::string text;
  for (const llm::Block& block : conversation.messages.back().blocks) {
    if (const auto* part = std::get_if<llm::TextBlock>(&block)) text += part->text;
  }
  return agent::Trimmed(text);
}

}  // namespace

agent::Section DiscordFormatSection() {
  return {"discord",
          "Your answers are shown in Discord, which renders a subset of Markdown. These work: **bold**, *italic*, __underline__, "
          "~~strikethrough~~, ||spoiler||, `inline code`, fenced code blocks with a language name, > quotes (>>> for several "
          "lines), lists that start with - or 1., headings with #, ## and ###, -# for small print, and links written as "
          "[text](url) (put a URL in <angle brackets> to stop Discord from adding a preview). These do not work and appear as "
          "plain characters: tables, image syntax, HTML, horizontal rules, math. Use lists instead of tables. A message holds "
          "at most 2000 characters, so keep answers short; a longer one is cut into several messages. Do not write @everyone, "
          "@here or <@...> mentions: nobody is notified by them. Call people by their names."};
}

std::string NeutralizeMentions(const std::string& text) {
  const std::string zero_width_space = "\xE2\x80\x8B";
  std::string out;
  std::size_t i = 0;
  while (i < text.size()) {
    if (text.compare(i, 9, "@everyone") == 0) {
      out += "@" + zero_width_space + "everyone";
      i += 9;
    } else if (text.compare(i, 5, "@here") == 0) {
      out += "@" + zero_width_space + "here";
      i += 5;
    } else if (text.compare(i, 2, "<@") == 0) {
      out += "<" + zero_width_space + "@";
      i += 2;
    } else {
      out += text[i++];
    }
  }
  return out;
}

// One channel's work: what is waiting, the answer being made, and (with Skills) its own tools.
struct Bot::Lane {
  struct Pending {
    std::string id;    // the message
    std::string text;  // as the model gets it (with the speaker in front, in a channel)
  };
  std::string channel_id;
  std::deque<Pending> inbox;
  bool running = false;
  int job_id = 0;
  int steps = 0;
  agent::TurnControl* control = nullptr;  // of the answer being made; null between answers
  std::unique_ptr<agent::ToolRuntime> runtime;
  std::shared_ptr<agent::SkillTools> skills;
};

// "typing..." is shown again every interval until this is stopped.
struct Bot::TypingState {
  explicit TypingState(const asio::any_io_executor& executor) : timer(executor) {}
  void Stop() {
    done = true;
    timer.cancel();
  }
  asio::steady_timer timer;
  bool done = false;
};

Bot::Bot(Gateway& gateway, http::Transport& transport, llm::Provider provider, std::shared_ptr<agent::History> history,
         BotOptions options, std::shared_ptr<agent::NativeTools> library, agent::SkillCatalog catalog)
    : gateway_(gateway),
      transport_(transport),
      provider_(std::move(provider)),
      history_(std::move(history)),
      options_(std::move(options)),
      library_(std::move(library)),
      catalog_(std::move(catalog)) {}

Bot::~Bot() = default;

std::size_t Bot::LaneCount() const { return lanes_.size(); }

std::string Bot::ErrorId() { return options_.make_error_id ? options_.make_error_id() : MakeErrorId(); }

bool Bot::Remember(const std::string& message_id) {
  if (!seen_.insert(message_id).second) return false;
  seen_order_.push_back(message_id);
  if (seen_order_.size() > 1000) {
    seen_.erase(seen_order_.front());
    seen_order_.pop_front();
  }
  return true;
}

Bot::Lane& Bot::LaneFor(const std::string& channel_id) {
  auto& slot = lanes_[channel_id];
  if (!slot) {
    slot = std::make_unique<Lane>();
    slot->channel_id = channel_id;
    if (library_ && !catalog_.skills.empty()) {
      slot->runtime = std::make_unique<agent::ToolRuntime>(options_.tools);
      slot->skills = std::make_shared<agent::SkillTools>(catalog_, library_, *slot->runtime);
      slot->runtime->AddSource(slot->skills);
    }
  }
  return *slot;
}

void Bot::Accept(IncomingMessage message) {
  if (stopping_ || stop_requested_) return;
  if (message.author_is_bot) return;
  if (!message.is_dm && !message.mentions_bot && !message.is_thread) return;
  const std::string text = agent::Trimmed(message.content);
  if (text.empty()) return;
  if (!Remember(message.id)) return;  // a reconnect can send the same message twice

  Lane& lane = LaneFor(message.channel_id);
  lane.inbox.push_back({message.id, message.is_dm ? text : message.author_name + ": " + text});
  if (!lane.running) {
    lane.running = true;
    ++active_jobs_;
    asio::co_spawn(executor_, RunLane(lane.channel_id), asio::detached);
  }
}

asio::awaitable<void> Bot::RunLane(std::string channel_id) {
  while (true) {
    Lane& lane = *lanes_.at(channel_id);
    if (lane.inbox.empty() || stopping_) break;
    // Everything that is waiting is one message for the model; the answer is a reply to the first of them.
    std::string text;
    const std::string reply_to = lane.inbox.front().id;
    for (const Lane::Pending& pending : lane.inbox) text += (text.empty() ? "" : "\n\n") + pending.text;
    lane.inbox.clear();
    co_await RunJob(lane, std::move(text), reply_to);
  }
  Lane& lane = *lanes_.at(channel_id);
  lane.running = false;
  --active_jobs_;
  const bool keeps_skills = lane.skills && !lane.skills->Loaded().empty();
  if (lane.inbox.empty() && !keeps_skills) lanes_.erase(channel_id);  // nothing uses `lane` after this
}

asio::awaitable<void> Bot::RunJob(Lane& lane, std::string text, std::string reply_to) {
  const std::string channel = lane.channel_id;
  lane.job_id = ++job_counter_;
  lane.steps = 0;
  spdlog::info("channel {}: job #{} starts", channel, lane.job_id);
  auto typing = std::make_shared<TypingState>(executor_);
  asio::co_spawn(executor_, TypingLoop(channel, typing), asio::detached);

  llm::Conversation conversation;
  std::size_t before = 0;
  std::optional<agent::TurnOutcome> outcome;
  std::string failure;
  try {
    conversation.messages = history_->Load(channel);
    before = conversation.messages.size();
    conversation.messages.push_back({llm::Role::kUser, {llm::TextBlock{text}}});

    agent::ContextPlan plan;
    plan.system = [this, &lane] {
      std::vector<agent::Section> sections = {{"", options_.system}, agent::RulesSection(lane.skills != nullptr)};
      if (lane.skills) sections.push_back(lane.skills->CatalogSection());
      sections.push_back(DiscordFormatSection());
      return sections;
    };
    plan.state = [this, &lane, channel] {
      agent::SessionInfo info;
      info.channel = channel;
      if (lane.skills) {
        info.has_skills = true;
        info.loaded_skills = lane.skills->Loaded();
        info.max_steps = options_.limits.max_steps;
      }
      const auto now = options_.now ? options_.now() : std::chrono::system_clock::now();  // the one place the clock is read
      const std::chrono::minutes offset = options_.utc_offset ? *options_.utc_offset : agent::LocalUtcOffset(now);
      return std::vector<agent::Section>{agent::SessionSection(info, now, offset)};
    };
    plan.budget_tokens = options_.budget_tokens > 0 ? options_.budget_tokens : agent::DefaultBudget(provider_, options_.llm);
    plan.count = options_.count;

    agent::Hooks hooks;
    if (lane.skills) hooks.push_back(lane.skills);  // gives the Skills back when the Turn is forgotten
    hooks.push_back(std::make_shared<StepHook>(&lane.steps));

    // What arrives while the Turn runs goes into it at its next Step.
    auto take = [&lane] {
      std::vector<std::string> texts;
      for (const Lane::Pending& pending : lane.inbox) texts.push_back(pending.text);
      lane.inbox.clear();
      return texts;
    };

    agent::TurnControl control;
    lane.control = &control;
    // The Turn runs in a coroutine of its own so that /stop cancels it and nothing else.
    outcome = co_await asio::co_spawn(
        executor_,
        agent::RunTurn(transport_, provider_, conversation, options_.llm, lane.runtime.get(), hooks, options_.limits, take, &plan),
        asio::bind_cancellation_slot(control.slot(), asio::use_awaitable));
    lane.control = nullptr;
  } catch (const std::exception& e) {
    lane.control = nullptr;
    failure = e.what();
  }
  typing->Stop();

  using Reason = agent::TurnOutcome::Reason;
  try {
    if (outcome && outcome->reason == Reason::kDone && !LastText(conversation).empty()) {
      history_->Append(channel, std::vector<llm::Message>(conversation.messages.begin() + static_cast<std::ptrdiff_t>(before),
                                                          conversation.messages.end()));
      co_await SendAll(channel, LastText(conversation), reply_to);
      spdlog::info("channel {}: job #{} answered in {} step(s)", channel, lane.job_id, outcome->steps);
    } else if (outcome && outcome->reason == Reason::kCancelled) {
      spdlog::info("channel {}: job #{} was stopped", channel, lane.job_id);
      co_await SendAll(channel, stopping_ ? kShuttingDown : kStopped, reply_to);
    } else {
      const Reason reason = outcome ? outcome->reason : Reason::kFailed;
      const std::string id = ErrorId();
      spdlog::error("[{}] channel {}: the answer failed ({}): {}", id, channel, ReasonName(reason),
                    outcome ? outcome->error : failure);
      co_await SendAll(channel, TroubleMessage(reason, id), reply_to);
    }
  } catch (const std::exception& e) {
    spdlog::error("channel {}: could not finish the answer: {}", channel, e.what());
  }
}

asio::awaitable<void> Bot::TypingLoop(std::string channel_id, std::shared_ptr<TypingState> state) {
  while (!state->done) {
    try {
      co_await gateway_.Typing(channel_id);
    } catch (const std::exception&) {
      // not being able to show "typing..." is not worth a failure
    }
    if (state->done) break;
    state->timer.expires_after(options_.typing_interval);
    boost::system::error_code ignored;
    co_await state->timer.async_wait(asio::redirect_error(asio::use_awaitable, ignored));
  }
}

asio::awaitable<void> Bot::SendAll(const std::string& channel_id, const std::string& text, const std::string& reply_to) {
  const std::vector<std::string> parts = SplitMessage(NeutralizeMentions(text));
  spdlog::info("channel {}: sending {} message(s)", channel_id, parts.size());
  bool first = true;
  for (const std::string& part : parts) {
    try {
      co_await gateway_.Send(channel_id, part, first ? reply_to : std::string());
    } catch (const std::exception& e) {
      spdlog::warn("channel {}: a message could not be sent: {}", channel_id, e.what());
      co_return;
    }
    first = false;
  }
}

asio::awaitable<void> Bot::HandleCommand(Command command) {
  std::string text;
  const auto found = lanes_.find(command.channel_id);
  Lane* lane = found == lanes_.end() ? nullptr : found->second.get();
  std::string skills;
  if (lane && lane->skills) {
    for (const std::string& name : lane->skills->Loaded()) skills += (skills.empty() ? "" : "、") + name;
  }
  if (command.name == "status") {
    if (lane && lane->running) {
      text = "作业 #" + std::to_string(lane->job_id) + " 正在运行，已经 " + std::to_string(lane->steps) + " 步；收件箱里有 " +
             std::to_string(lane->inbox.size()) + " 条消息";
    } else {
      text = "这个频道现在没有作业";
    }
    if (!skills.empty()) text += "；已加载的 Skill：" + skills;
    text += "。";
  } else if (command.name == "stop") {
    if (lane && lane->running && lane->control != nullptr) {
      lane->control->Cancel();
      text = "已请求停止。";
    } else {
      text = "这个频道没有正在运行的作业。";
    }
  } else {
    text = "不认识的命令。";
  }
  spdlog::info("channel {}: the command {} is answered: {}", command.channel_id, command.name, text);
  try {
    co_await gateway_.Reply(command, text);
  } catch (const std::exception& e) {
    spdlog::warn("a command could not be answered: {}", e.what());
  }
}

asio::awaitable<void> Bot::Run() {
  executor_ = co_await asio::this_coro::executor;
  stop_timer_ = std::make_unique<asio::steady_timer>(executor_, asio::steady_timer::time_point::max());
  gateway_.OnMessage([this](IncomingMessage message) {
    try {
      Accept(std::move(message));
    } catch (const std::exception& e) {
      spdlog::error("a message could not be taken: {}", e.what());
    }
  });
  gateway_.OnCommand([this](Command command) { asio::co_spawn(executor_, HandleCommand(std::move(command)), asio::detached); });
  gateway_.OnFatal([this](std::string why) {
    if (!failure_) failure_ = std::move(why);
    RequestStop();
  });
  gateway_.Start();

  if (!stop_requested_) {
    boost::system::error_code ignored;
    co_await stop_timer_->async_wait(asio::redirect_error(asio::use_awaitable, ignored));  // RequestStop ends the wait
  }

  stopping_ = true;
  for (auto& entry : lanes_) {
    if (entry.second->control != nullptr) entry.second->control->Cancel();
  }
  asio::steady_timer poll(executor_);
  const auto give_up = std::chrono::steady_clock::now() + options_.shutdown_wait;
  while (active_jobs_ > 0 && std::chrono::steady_clock::now() < give_up) {
    poll.expires_after(std::chrono::milliseconds(10));
    boost::system::error_code ignored;
    co_await poll.async_wait(asio::redirect_error(asio::use_awaitable, ignored));
  }
  if (active_jobs_ > 0) spdlog::warn("{} answers were still being made when the bot stopped", active_jobs_);
  co_await gateway_.Stop();
}

void Bot::RequestStop() {
  stop_requested_ = true;
  if (stop_timer_) stop_timer_->cancel();
}

}  // namespace bot
