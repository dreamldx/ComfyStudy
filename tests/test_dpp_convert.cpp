#include "dpp_convert.h"

#include <gtest/gtest.h>

#include <string>

namespace {

dpp::message Message(const std::string& content) {
  dpp::message message;
  message.id = dpp::snowflake(111);
  message.channel_id = dpp::snowflake(222);
  message.guild_id = dpp::snowflake(333);
  message.author.id = dpp::snowflake(444);
  message.author.username = "mei_user";
  message.author.global_name = "Mei";
  message.content = content;
  return message;
}

const dpp::snowflake kBot(999);

}  // namespace

TEST(StripBotMention, TakesOutBothFormsOfTheMentionAndTheBlankSpaceAroundWhatIsLeft) {
  EXPECT_EQ(bot::StripBotMention("<@999> hello", "999"), "hello");
  EXPECT_EQ(bot::StripBotMention("<@!999>  hello there \n", "999"), "hello there");
  EXPECT_EQ(bot::StripBotMention("hi <@999> you", "999"), "hi  you");
  EXPECT_EQ(bot::StripBotMention("<@999>", "999"), "");
}

TEST(StripBotMention, LeavesTheMentionsOfOtherPeopleAlone) {
  EXPECT_EQ(bot::StripBotMention("<@999> ask <@123>", "999"), "ask <@123>");
  EXPECT_EQ(bot::StripBotMention("<@9999> not me", "999"), "<@9999> not me");
}

TEST(ConvertMessage, TheIdsTheNameAndTheText) {
  const bot::IncomingMessage message = bot::ConvertMessage(Message("hello"), kBot, false);
  EXPECT_EQ(message.id, "111");
  EXPECT_EQ(message.channel_id, "222");
  EXPECT_EQ(message.author_id, "444");
  EXPECT_EQ(message.author_name, "Mei");  // the display name when there is one
  EXPECT_EQ(message.content, "hello");
  EXPECT_FALSE(message.author_is_bot);
  EXPECT_FALSE(message.mentions_bot);
  EXPECT_FALSE(message.is_dm);
  EXPECT_FALSE(message.is_thread);
}

TEST(ConvertMessage, WithoutADisplayNameTheUserNameIsUsed) {
  dpp::message message = Message("hello");
  message.author.global_name.clear();
  EXPECT_EQ(bot::ConvertMessage(message, kBot, false).author_name, "mei_user");
}

TEST(ConvertMessage, AMentionInTheTextOrInTheListOfMentionsMakesItAMentionAndIsTakenOutOfTheText) {
  const bot::IncomingMessage in_text = bot::ConvertMessage(Message("<@999> what time is it"), kBot, false);
  EXPECT_TRUE(in_text.mentions_bot);
  EXPECT_EQ(in_text.content, "what time is it");

  dpp::message listed = Message("what time is it");
  dpp::user bot_user;
  bot_user.id = kBot;
  listed.mentions.push_back({bot_user, dpp::guild_member()});
  EXPECT_TRUE(bot::ConvertMessage(listed, kBot, false).mentions_bot);

  dpp::message other = Message("<@123> what time is it");
  dpp::user someone;
  someone.id = dpp::snowflake(123);
  other.mentions.push_back({someone, dpp::guild_member()});
  EXPECT_FALSE(bot::ConvertMessage(other, kBot, false).mentions_bot);
}

TEST(ConvertMessage, ABotAuthorAndAMessageWithoutAGuildAreSaidSo) {
  dpp::message message = Message("hello");
  message.author.flags |= dpp::u_bot;
  message.guild_id = dpp::snowflake(0);
  const bot::IncomingMessage converted = bot::ConvertMessage(message, kBot, false);
  EXPECT_TRUE(converted.author_is_bot);
  EXPECT_TRUE(converted.is_dm);
}

TEST(ConvertMessage, ThePlaceOfTheMessageIsGivenByTheCaller) {
  EXPECT_TRUE(bot::ConvertMessage(Message("hello"), kBot, true).is_thread);
}

TEST(IsThreadType, TheThreeKindsOfThreadAndNothingElse) {
  EXPECT_TRUE(bot::IsThreadType(dpp::CHANNEL_PUBLIC_THREAD));
  EXPECT_TRUE(bot::IsThreadType(dpp::CHANNEL_PRIVATE_THREAD));
  EXPECT_TRUE(bot::IsThreadType(dpp::CHANNEL_ANNOUNCEMENT_THREAD));
  EXPECT_FALSE(bot::IsThreadType(dpp::CHANNEL_TEXT));
  EXPECT_FALSE(bot::IsThreadType(dpp::CHANNEL_CATEGORY));
}

TEST(FatalGatewayProblem, ADisallowedIntentTellsWhereToSwitchItOn) {
  const auto problem = bot::FatalGatewayProblem("Receiving exception: OOF! Error from underlying websocket: 4014: Disallowed intent(s)");
  ASSERT_TRUE(problem.has_value());
  EXPECT_NE(problem->find("MESSAGE CONTENT INTENT"), std::string::npos) << *problem;
  EXPECT_NE(problem->find("developer portal"), std::string::npos) << *problem;
}

TEST(FatalGatewayProblem, ARefusedTokenSaysSo) {
  const auto problem = bot::FatalGatewayProblem("Receiving exception: OOF! Error from underlying websocket: 4004: Authentication failed.");
  ASSERT_TRUE(problem.has_value());
  EXPECT_NE(problem->find("discord_token"), std::string::npos) << *problem;
}

TEST(FatalGatewayProblem, OrdinaryLogLinesAreNotFatal) {
  EXPECT_FALSE(bot::FatalGatewayProblem("Connecting new session...").has_value());
  EXPECT_FALSE(bot::FatalGatewayProblem("Lost connection to websocket on shard 0, reconnecting...").has_value());
  EXPECT_FALSE(bot::FatalGatewayProblem("").has_value());
}
