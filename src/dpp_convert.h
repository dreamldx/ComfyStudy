#pragma once

#include <dpp/dpp.h>

#include <optional>
#include <string>

#include "gateway.h"

// What the Discord side of the bot does with DPP's types, apart from the connection: kept in its own file so that
// a test can build a dpp::message by hand and see what the bot would be told.
namespace bot {

// `content` without the mentions of the bot (<@id> and <@!id>), and without the blank space around what is left.
std::string StripBotMention(const std::string& content, const std::string& bot_id);

// Is this channel type a thread?
bool IsThreadType(dpp::channel_type type);

// When a line of DPP's log says that the connection cannot work and will not by itself (an intent that is not
// switched on in the developer portal, a token that was refused), a sentence for the user that says what to do;
// for any other line nothing.
std::optional<std::string> FatalGatewayProblem(const std::string& log_message);

// The message as the bot sees it. `in_thread` says whether its channel is a thread (the message does not say).
IncomingMessage ConvertMessage(const dpp::message& message, const dpp::snowflake& bot_id, bool in_thread);

}  // namespace bot
