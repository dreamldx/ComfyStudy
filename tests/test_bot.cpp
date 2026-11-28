#include "bot.h"

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "json.h"

#include "fake_gateway.h"
#include "gated_transport.h"
#include "scripted_transport.h"  // AnthropicBody

namespace {

using Json = json::Value;
using namespace std::chrono_literals;

bool Contains(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

llm::Reply Said(const std::string& text) {
  llm::Reply reply;
  reply.message.role = llm::Role::kAssistant;
  reply.message.blocks.push_back(llm::TextBlock{text});
  reply.stop_reason = "end_turn";
  return reply;
}

llm::Reply Calls(const std::string& id, const std::string& name, Json arguments) {
  llm::Reply reply;
  reply.message.role = llm::Role::kAssistant;
  reply.message.blocks.push_back(llm::ToolCallBlock{id, name, std::move(arguments), "", ""});
  reply.stop_reason = "tool_use";
  return reply;
}

http::Response Ok(const llm::Reply& reply) { return {200, AnthropicBody(reply)}; }

bot::IncomingMessage Mention(const std::string& id, const std::string& channel, const std::string& text, const std::string& name = "Mei") {
  bot::IncomingMessage message;
  message.id = id;
  message.channel_id = channel;
  message.author_id = "u-" + name;
  message.author_name = name;
  message.mentions_bot = true;
  message.content = text;
  return message;
}

// The text of the last user message of a request, and the blocks of the first one.
Json LastUser(const std::string& body) {
  const Json request = json::Parse(body);
  const Json& messages = request["messages"];
  return messages[messages.size() - 1];
}
std::string LastUserText(const std::string& body) {
  const Json message = LastUser(body);
  return message["content"][message["content"].size() - 1]["text"].get<std::string>();
}

// A bot on a fake gateway and a transport that waits for the test, run on one io_context.
struct World {
  explicit World(bool skills = false) {
    options.now = [] { return std::chrono::sys_days{std::chrono::year{2026} / 11 / 28} + std::chrono::hours{12}; };
    options.utc_offset = std::chrono::minutes(0);
    options.make_error_id = [] { return std::string("ABC234"); };
    options.typing_interval = 20ms;
    options.shutdown_wait = 500ms;
    options.system = "Be brief.";
    if (skills) {
      library = std::make_shared<agent::NativeTools>();
      agent::AddToyTools(*library);
      catalog.skills = {agent::SkillInfo{"calculator", "exact arithmetic", {"add"}, "Use add for sums.", {}}};
    }
  }
  ~World() {
    if (bot && !finished) {
      bot->RequestStop();
      Until([&] { return finished; });
    }
  }

  void Start() {
    bot = std::make_unique<bot::Bot>(gateway, transport, provider, history, options, library, catalog);
    boost::asio::co_spawn(
        io,
        [this]() -> boost::asio::awaitable<void> {
          co_await bot->Run();
          finished = true;
        },
        boost::asio::detached);
    Until([&] { return gateway.started; });
  }

  bool Until(const std::function<bool()>& done, std::chrono::milliseconds timeout = 3000ms) {
    const auto end = std::chrono::steady_clock::now() + timeout;
    while (!done()) {
      if (std::chrono::steady_clock::now() > end) return false;
      io.restart();
      io.run_for(5ms);
    }
    return true;
  }
  void Wait(std::chrono::milliseconds duration) {
    const auto end = std::chrono::steady_clock::now() + duration;
    while (std::chrono::steady_clock::now() < end) {
      io.restart();
      io.run_for(5ms);
    }
  }
  bool Requests(std::size_t n) { return Until([&] { return transport.Count() >= n; }); }
  bool Sent(std::size_t n) { return Until([&] { return gateway.sent.size() >= n; }); }
  void Answer(std::size_t index, const std::string& text) { transport.Release(index, Ok(Said(text))); }

  boost::asio::io_context io;
  FakeGateway gateway;
  GatedTransport transport;
  std::shared_ptr<agent::MemoryHistory> history = std::make_shared<agent::MemoryHistory>();
  llm::Provider provider{"anthropic", llm::Protocol::kAnthropic, llm::Endpoint{"https://a.test", "key"}, "m", {true, 1000, 100000}};
  bot::BotOptions options;
  std::shared_ptr<agent::NativeTools> library;
  agent::SkillCatalog catalog;
  std::unique_ptr<bot::Bot> bot;
  bool finished = false;
};

}  // namespace

// --- Who is answered ------------------------------------------------------------------------------

TEST(Bot, AMessageThatMentionsTheBotIsAnsweredWithAReplyToIt) {
  World w;
  w.Start();
  w.gateway.Deliver(Mention("m1", "c1", "hello"));
  ASSERT_TRUE(w.Requests(1));
  w.Answer(0, "Hi there");
  ASSERT_TRUE(w.Sent(1));
  EXPECT_EQ(w.gateway.sent[0].channel, "c1");
  EXPECT_EQ(w.gateway.sent[0].text, "Hi there");
  EXPECT_EQ(w.gateway.sent[0].reply_to, "m1");
}

TEST(Bot, InAChannelTheSpeakerIsNamedInFrontOfTheTextButInAPrivateMessageItIsJustTheText) {
  World w;
  w.Start();
  w.gateway.Deliver(Mention("m1", "c1", "hello", "Mei"));
  bot::IncomingMessage dm = Mention("m2", "dm1", "psst", "Li");
  dm.mentions_bot = false;
  dm.is_dm = true;
  w.gateway.Deliver(dm);
  ASSERT_TRUE(w.Requests(2));
  EXPECT_EQ(LastUserText(w.transport.Body(0)), "Mei: hello");
  EXPECT_EQ(LastUserText(w.transport.Body(1)), "psst");
}

TEST(Bot, AMessageInAChannelThatDoesNotMentionTheBotIsIgnored) {
  World w;
  w.Start();
  bot::IncomingMessage message = Mention("m1", "c1", "hello");
  message.mentions_bot = false;
  w.gateway.Deliver(message);
  w.Wait(100ms);
  EXPECT_EQ(w.transport.Count(), 0u);
  EXPECT_EQ(w.bot->LaneCount(), 0u);
}

TEST(Bot, EveryMessageInAThreadAndInAPrivateMessageIsAnswered) {
  World w;
  w.Start();
  bot::IncomingMessage thread = Mention("m1", "t1", "in a thread");
  thread.mentions_bot = false;
  thread.is_thread = true;
  w.gateway.Deliver(thread);
  bot::IncomingMessage dm = Mention("m2", "d1", "private");
  dm.mentions_bot = false;
  dm.is_dm = true;
  w.gateway.Deliver(dm);
  EXPECT_TRUE(w.Requests(2));
}

TEST(Bot, TheBotsOwnMessagesAndThoseOfOtherBotsAreIgnored) {
  World w;
  w.Start();
  bot::IncomingMessage message = Mention("m1", "c1", "hello");
  message.author_is_bot = true;
  w.gateway.Deliver(message);
  w.Wait(100ms);
  EXPECT_EQ(w.transport.Count(), 0u);
}

TEST(Bot, AMessageWithNothingButTheMentionIsIgnored) {
  World w;
  w.Start();
  w.gateway.Deliver(Mention("m1", "c1", "   "));
  w.Wait(100ms);
  EXPECT_EQ(w.transport.Count(), 0u);
}

TEST(Bot, TheSameMessageDeliveredAgainIsNotAnsweredAgain) {
  World w;
  w.Start();
  w.gateway.Deliver(Mention("m1", "c1", "hello"));
  ASSERT_TRUE(w.Requests(1));
  w.Answer(0, "Hi");
  ASSERT_TRUE(w.Sent(1));
  w.gateway.Deliver(Mention("m1", "c1", "hello"));  // a reconnect sent it again
  w.Wait(100ms);
  EXPECT_EQ(w.transport.Count(), 1u);
  EXPECT_EQ(w.gateway.sent.size(), 1u);
}

// --- Order and lanes ------------------------------------------------------------------------------

TEST(Bot, WhatArrivesWhileAnAnswerIsBeingMadeWaitsAndThenIsOneMessageForTheNextAnswer) {
  World w;
  w.Start();
  w.gateway.Deliver(Mention("m1", "c1", "first"));
  ASSERT_TRUE(w.Requests(1));
  w.gateway.Deliver(Mention("m2", "c1", "second"));
  w.gateway.Deliver(Mention("m3", "c1", "third", "Li"));
  w.Wait(100ms);
  EXPECT_EQ(w.transport.Count(), 1u);  // one answer at a time in a channel
  w.Answer(0, "one");
  ASSERT_TRUE(w.Requests(2));
  EXPECT_EQ(LastUserText(w.transport.Body(1)), "Mei: second\n\nLi: third");
  EXPECT_EQ(w.gateway.sent.at(0).text, "one");
  w.Answer(1, "two and three");
  ASSERT_TRUE(w.Sent(2));
  EXPECT_EQ(w.gateway.sent[1].reply_to, "m2");  // the answer is to the first of the messages it covers
}

TEST(Bot, AMessageThatArrivesDuringAnAnswerIsGivenToTheModelAtTheNextStep) {
  World w(true);
  w.Start();
  w.gateway.Deliver(Mention("m1", "c1", "please add"));
  ASSERT_TRUE(w.Requests(1));
  w.gateway.Deliver(Mention("m2", "c1", "and be quick"));
  w.transport.Release(0, Ok(Calls("t1", "load_skill", Json{{"name", "calculator"}})));
  ASSERT_TRUE(w.Requests(2));
  const std::string second = w.transport.Body(1);
  EXPECT_TRUE(Contains(second, agent::kSteeringMarker));
  EXPECT_TRUE(Contains(second, "Mei: and be quick"));
  w.Answer(1, "done");
  ASSERT_TRUE(w.Sent(1));
  w.Wait(100ms);
  EXPECT_EQ(w.transport.Count(), 2u);  // the message was taken by the Turn, it does not start another
}

TEST(Bot, DifferentChannelsAreAnsweredAtTheSameTime) {
  World w;
  w.Start();
  w.gateway.Deliver(Mention("m1", "c1", "hello"));
  w.gateway.Deliver(Mention("m2", "c2", "hello"));
  ASSERT_TRUE(w.Requests(2));  // both are in flight, nothing has been released
  w.Answer(1, "for c2");
  w.Answer(0, "for c1");
  ASSERT_TRUE(w.Sent(2));
  for (const auto& sent : w.gateway.sent) {
    EXPECT_TRUE((sent.channel == "c1" && sent.text == "for c1") || (sent.channel == "c2" && sent.text == "for c2"));
  }
}

TEST(Bot, EachChannelHasItsOwnHistory) {
  World w;
  w.Start();
  w.gateway.Deliver(Mention("m1", "c1", "one"));
  w.gateway.Deliver(Mention("m2", "c2", "two"));
  ASSERT_TRUE(w.Requests(2));
  w.Answer(0, "a");
  w.Answer(1, "b");
  ASSERT_TRUE(w.Sent(2));
  EXPECT_EQ(w.history->Load("c1").size(), 2u);
  EXPECT_EQ(w.history->Load("c2").size(), 2u);
  w.gateway.Deliver(Mention("m3", "c1", "again"));
  ASSERT_TRUE(w.Requests(3));
  const Json request = json::Parse(w.transport.Body(2));
  EXPECT_EQ(request["messages"].size(), 3u);  // c1's question and answer, and the new one; nothing of c2's
}

TEST(Bot, ALaneIsGoneWhenItsWorkIsDoneUnlessASkillIsLoadedInIt) {
  World plain;
  plain.Start();
  plain.gateway.Deliver(Mention("m1", "c1", "hi"));
  ASSERT_TRUE(plain.Requests(1));
  EXPECT_EQ(plain.bot->LaneCount(), 1u);
  plain.Answer(0, "ok");
  ASSERT_TRUE(plain.Sent(1));
  EXPECT_TRUE(plain.Until([&] { return plain.bot->LaneCount() == 0; }));

  World skills(true);
  skills.Start();
  skills.gateway.Deliver(Mention("m1", "c1", "add"));
  ASSERT_TRUE(skills.Requests(1));
  skills.transport.Release(0, Ok(Calls("t1", "load_skill", Json{{"name", "calculator"}})));
  ASSERT_TRUE(skills.Requests(2));
  skills.Answer(1, "loaded");
  ASSERT_TRUE(skills.Sent(1));
  skills.Wait(100ms);
  EXPECT_EQ(skills.bot->LaneCount(), 1u);  // it keeps what it loaded
}

// --- What goes to the model ------------------------------------------------------------------------

TEST(Bot, TheSystemPromptHasTheInstructionTheRulesTheSkillsAndHowDiscordShowsText) {
  World w(true);
  w.Start();
  w.gateway.Deliver(Mention("m1", "c1", "hi"));
  ASSERT_TRUE(w.Requests(1));
  const std::string system = json::Parse(w.transport.Body(0))["system"].get<std::string>();
  EXPECT_EQ(system.rfind("Be brief.", 0), 0u);
  const std::size_t rules = system.find("[rules]");
  const std::size_t skills = system.find("[skills]");
  const std::size_t discord = system.find("[discord]");
  ASSERT_NE(rules, std::string::npos);
  ASSERT_NE(skills, std::string::npos);
  ASSERT_NE(discord, std::string::npos);
  EXPECT_LT(rules, skills);
  EXPECT_LT(skills, discord);
  EXPECT_TRUE(Contains(system, "- calculator: exact arithmetic"));
}

TEST(Bot, WithoutSkillsThereIsNoListOfThemAndNoTools) {
  World w;
  w.Start();
  w.gateway.Deliver(Mention("m1", "c1", "hi"));
  ASSERT_TRUE(w.Requests(1));
  const Json request = json::Parse(w.transport.Body(0));
  EXPECT_FALSE(Contains(request["system"].get<std::string>(), "[skills]"));
  EXPECT_FALSE(request.contains("tools"));
}

TEST(Bot, TheStateNamesTheChannelAndTheDate) {
  World w;
  w.Start();
  w.gateway.Deliver(Mention("m1", "c-42", "hi"));
  ASSERT_TRUE(w.Requests(1));
  const Json first = json::Parse(w.transport.Body(0))["messages"][0]["content"][0];
  const std::string state = first["text"].get<std::string>();
  EXPECT_TRUE(Contains(state, "[session state]"));
  EXPECT_TRUE(Contains(state, "channel: c-42"));
  EXPECT_TRUE(Contains(state, "date: 2026-11-28 Saturday (UTC+00:00)"));
}

TEST(Bot, TheDiscordFormatSectionSaysWhatWorksAndWhatDoesNot) {
  const agent::Section section = bot::DiscordFormatSection();
  EXPECT_EQ(section.title, "discord");
  for (const char* part : {"bold", "code", "2000", "table", "@everyone"}) EXPECT_TRUE(Contains(section.text, part)) << part;
}

// --- What is sent back -----------------------------------------------------------------------------

TEST(Bot, AnAnswerOverTheLimitIsSentInSeveralMessagesAndOnlyTheFirstIsAReply) {
  World w;
  w.Start();
  w.gateway.Deliver(Mention("m1", "c1", "long please"));
  ASSERT_TRUE(w.Requests(1));
  std::string answer;
  for (int i = 0; i < 300; ++i) answer += "word" + std::to_string(i) + " ";
  w.Answer(0, answer);
  ASSERT_TRUE(w.Sent(2));
  EXPECT_EQ(w.gateway.sent[0].reply_to, "m1");
  EXPECT_EQ(w.gateway.sent[1].reply_to, "");
  for (const auto& sent : w.gateway.sent) EXPECT_LE(sent.text.size(), 2000u);
}

TEST(Bot, MentionsInTheAnswerAreNotMentions) {
  EXPECT_EQ(bot::NeutralizeMentions("hi @everyone and @here"), std::string("hi @\xE2\x80\x8B" "everyone and @\xE2\x80\x8B" "here"));
  EXPECT_EQ(bot::NeutralizeMentions("<@123> <@!456> <@&789>"), std::string("<\xE2\x80\x8B@123> <\xE2\x80\x8B@!456> <\xE2\x80\x8B@&789>"));
  EXPECT_EQ(bot::NeutralizeMentions("no mentions here"), "no mentions here");

  World w;
  w.Start();
  w.gateway.Deliver(Mention("m1", "c1", "hi"));
  ASSERT_TRUE(w.Requests(1));
  w.Answer(0, "Hello @everyone");
  ASSERT_TRUE(w.Sent(1));
  EXPECT_FALSE(Contains(w.gateway.sent[0].text, "@everyone"));
}

TEST(Bot, WhileAnAnswerIsBeingMadeTheChannelShowsTypingAndWhenItIsDoneItStops) {
  World w;
  w.Start();
  w.gateway.Deliver(Mention("m1", "c1", "hi"));
  ASSERT_TRUE(w.Requests(1));
  ASSERT_TRUE(w.Until([&] { return w.gateway.typing["c1"] >= 3; }));  // it is shown again every interval
  w.Answer(0, "ok");
  ASSERT_TRUE(w.Sent(1));
  w.Wait(30ms);
  const int after = w.gateway.typing["c1"];
  w.Wait(100ms);
  EXPECT_EQ(w.gateway.typing["c1"], after);
}

TEST(Bot, AMessageThatCouldNotBeSentDoesNotStopTheBot) {
  World w;
  w.Start();
  w.gateway.fail_sends = 1;
  w.gateway.Deliver(Mention("m1", "c1", "hi"));
  ASSERT_TRUE(w.Requests(1));
  w.Answer(0, "lost");
  w.Wait(100ms);
  EXPECT_TRUE(w.gateway.sent.empty());
  w.gateway.Deliver(Mention("m2", "c1", "again"));
  ASSERT_TRUE(w.Requests(2));
  w.Answer(1, "kept");
  ASSERT_TRUE(w.Sent(1));
  EXPECT_EQ(w.gateway.sent[0].text, "kept");
}

// --- When it goes wrong --------------------------------------------------------------------------------

TEST(Bot, WhenTheServiceFailsTheUserGetsAnErrorIdAndNothingFromInside) {
  World w;
  w.Start();
  w.gateway.Deliver(Mention("m1", "c1", "hi"));
  ASSERT_TRUE(w.Requests(1));
  w.transport.Release(0, {500, R"({"error": {"message": "secret path C:\\private\\key.pem"}})"});
  ASSERT_TRUE(w.Sent(1));
  const std::string text = w.gateway.sent[0].text;
  EXPECT_TRUE(Contains(text, "ABC234")) << text;
  EXPECT_FALSE(Contains(text, "secret")) << text;
  EXPECT_FALSE(Contains(text, "private")) << text;
  EXPECT_FALSE(Contains(text, "500")) << text;
  EXPECT_EQ(w.gateway.sent[0].reply_to, "m1");
}

TEST(Bot, ATurnThatFailsLeavesNothingInTheHistory) {
  World w;
  w.Start();
  w.gateway.Deliver(Mention("m1", "c1", "hi"));
  ASSERT_TRUE(w.Requests(1));
  w.transport.Release(0, {500, "no"});
  ASSERT_TRUE(w.Sent(1));
  EXPECT_TRUE(w.history->Load("c1").empty());
}

// --- Commands ---------------------------------------------------------------------------------------

TEST(Bot, StopCancelsTheAnswerThatIsBeingMadeAndTheChannelSaysSoAndTheHistoryIsUntouched) {
  World w;
  w.Start();
  w.gateway.Deliver(Mention("m1", "c1", "a long job"));
  ASSERT_TRUE(w.Requests(1));
  w.gateway.Use(bot::Command{"stop", "c1", "u1", "h1"});
  ASSERT_TRUE(w.Sent(1));
  ASSERT_EQ(w.gateway.replies.size(), 1u);
  EXPECT_TRUE(Contains(w.gateway.replies[0].text, "停止"));
  EXPECT_EQ(w.gateway.replies[0].command.handle, "h1");
  EXPECT_TRUE(Contains(w.gateway.sent[0].text, "停止"));
  EXPECT_TRUE(w.history->Load("c1").empty());
}

TEST(Bot, WhatWasWaitingWhenAnAnswerWasStoppedBecomesTheNextAnswer) {
  World w;
  w.Start();
  w.gateway.Deliver(Mention("m1", "c1", "a long job"));
  ASSERT_TRUE(w.Requests(1));
  w.gateway.Deliver(Mention("m2", "c1", "never mind, do this"));
  w.gateway.Use(bot::Command{"stop", "c1", "u1", "h1"});
  ASSERT_TRUE(w.Requests(2));
  EXPECT_EQ(LastUserText(w.transport.Body(1)), "Mei: never mind, do this");
}

TEST(Bot, StopWithNothingRunningSaysThereIsNothing) {
  World w;
  w.Start();
  w.gateway.Use(bot::Command{"stop", "c1", "u1", "h1"});
  ASSERT_TRUE(w.Until([&] { return !w.gateway.replies.empty(); }));
  EXPECT_TRUE(Contains(w.gateway.replies[0].text, "没有"));
}

TEST(Bot, StatusSaysWhatIsHappeningInTheChannel) {
  World w(true);
  w.Start();
  w.gateway.Use(bot::Command{"status", "c1", "u1", "h1"});
  ASSERT_TRUE(w.Until([&] { return w.gateway.replies.size() == 1; }));
  EXPECT_TRUE(Contains(w.gateway.replies[0].text, "没有作业")) << w.gateway.replies[0].text;

  w.gateway.Deliver(Mention("m1", "c1", "a job"));
  ASSERT_TRUE(w.Requests(1));
  w.gateway.Deliver(Mention("m2", "c1", "one more"));
  w.gateway.Use(bot::Command{"status", "c1", "u1", "h2"});
  ASSERT_TRUE(w.Until([&] { return w.gateway.replies.size() == 2; }));
  const std::string text = w.gateway.replies[1].text;
  EXPECT_TRUE(Contains(text, "#1")) << text;
  EXPECT_TRUE(Contains(text, "1 步")) << text;
  EXPECT_TRUE(Contains(text, "1 条")) << text;
}

TEST(Bot, StatusNamesTheSkillsThatAreLoaded) {
  World w(true);
  w.Start();
  w.gateway.Deliver(Mention("m1", "c1", "add"));
  ASSERT_TRUE(w.Requests(1));
  w.transport.Release(0, Ok(Calls("t1", "load_skill", Json{{"name", "calculator"}})));
  ASSERT_TRUE(w.Requests(2));
  w.Answer(1, "ok");
  ASSERT_TRUE(w.Sent(1));
  w.gateway.Use(bot::Command{"status", "c1", "u1", "h1"});
  ASSERT_TRUE(w.Until([&] { return !w.gateway.replies.empty(); }));
  EXPECT_TRUE(Contains(w.gateway.replies[0].text, "calculator")) << w.gateway.replies[0].text;
}

// --- Stopping -------------------------------------------------------------------------------------------

TEST(Bot, StoppingCancelsTheAnswersThatAreBeingMadeAndLeavesNoHalfHistory) {
  World w;
  w.Start();
  w.gateway.Deliver(Mention("m1", "c1", "hi"));
  ASSERT_TRUE(w.Requests(1));
  w.bot->RequestStop();
  ASSERT_TRUE(w.Until([&] { return w.finished; }));
  EXPECT_TRUE(w.gateway.stopped);
  EXPECT_TRUE(w.history->Load("c1").empty());
  ASSERT_FALSE(w.gateway.sent.empty());
  EXPECT_TRUE(Contains(w.gateway.sent[0].text, "关闭"));
}

TEST(Bot, StoppingWhenNothingIsRunningIsQuick) {
  World w;
  w.Start();
  w.bot->RequestStop();
  EXPECT_TRUE(w.Until([&] { return w.finished; }, 500ms));
  EXPECT_TRUE(w.gateway.stopped);
}

TEST(Bot, AfterAStopIsAskedForNothingNewIsTaken) {
  World w;
  w.Start();
  w.gateway.Deliver(Mention("m1", "c1", "hi"));
  ASSERT_TRUE(w.Requests(1));
  w.bot->RequestStop();
  w.gateway.Deliver(Mention("m2", "c2", "too late"));
  ASSERT_TRUE(w.Until([&] { return w.finished; }));
  EXPECT_EQ(w.transport.Count(), 1u);
}

TEST(Bot, AProblemThatMakesTheGatewayUnusableStopsTheBotAndIsReported) {
  World w;
  w.Start();
  EXPECT_FALSE(w.bot->Failure().has_value());
  w.gateway.Fail("the token was refused");
  ASSERT_TRUE(w.Until([&] { return w.finished; }));
  ASSERT_TRUE(w.bot->Failure().has_value());
  EXPECT_EQ(*w.bot->Failure(), "the token was refused");
  EXPECT_TRUE(w.gateway.stopped);
}

TEST(Bot, AnOrdinaryStopIsNotAFailure) {
  World w;
  w.Start();
  w.bot->RequestStop();
  ASSERT_TRUE(w.Until([&] { return w.finished; }));
  EXPECT_FALSE(w.bot->Failure().has_value());
}
