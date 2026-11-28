#include "dpp_convert.h"

#include <algorithm>

namespace bot {

std::string StripBotMention(const std::string& content, const std::string& bot_id) {
  std::string out = content;
  for (const std::string& mention : {"<@" + bot_id + ">", "<@!" + bot_id + ">"}) {
    for (std::size_t at = out.find(mention); at != std::string::npos; at = out.find(mention)) out.erase(at, mention.size());
  }
  std::size_t first = 0;
  std::size_t last = out.size();
  while (first < last && (out[first] == ' ' || out[first] == '\n' || out[first] == '\r' || out[first] == '\t')) ++first;
  while (last > first && (out[last - 1] == ' ' || out[last - 1] == '\n' || out[last - 1] == '\r' || out[last - 1] == '\t')) --last;
  return out.substr(first, last - first);
}

bool IsThreadType(dpp::channel_type type) {
  return type == dpp::CHANNEL_PUBLIC_THREAD || type == dpp::CHANNEL_PRIVATE_THREAD || type == dpp::CHANNEL_ANNOUNCEMENT_THREAD;
}

std::optional<std::string> FatalGatewayProblem(const std::string& log_message) {
  const auto has = [&](const char* part) { return log_message.find(part) != std::string::npos; };
  if (has("4014") || has("Disallowed intent")) {
    return "Discord refused the connection: the bot asks for an intent that is not switched on. Open the Discord developer portal, "
           "your application, Bot, and switch on MESSAGE CONTENT INTENT (under Privileged Gateway Intents); then start the bot again.";
  }
  if (has("4004") || has("Authentication failed")) {
    return "Discord refused the token (authentication failed). Check the setting discord_token: it has to be the token of the bot, "
           "from the Bot page of your application in the developer portal.";
  }
  if (has("4013") || has("Invalid intent")) {
    return "Discord refused the connection: the intents the bot asks for are not valid.";
  }
  return std::nullopt;
}

IncomingMessage ConvertMessage(const dpp::message& message, const dpp::snowflake& bot_id, bool in_thread) {
  IncomingMessage out;
  const std::string bot = bot_id.str();
  out.id = message.id.str();
  out.channel_id = message.channel_id.str();
  out.author_id = message.author.id.str();
  out.author_name = message.author.global_name.empty() ? message.author.username : message.author.global_name;
  out.author_is_bot = message.author.is_bot();
  out.is_dm = message.is_dm();
  out.is_thread = in_thread;
  const bool listed = std::any_of(message.mentions.begin(), message.mentions.end(),
                                  [&](const std::pair<dpp::user, dpp::guild_member>& mention) { return mention.first.id == bot_id; });
  out.mentions_bot = listed || message.content.find("<@" + bot + ">") != std::string::npos ||
                     message.content.find("<@!" + bot + ">") != std::string::npos;
  out.content = StripBotMention(message.content, bot);
  return out;
}

}  // namespace bot
