#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

#include "config.h"
#include "conversation.h"
#include "provider.h"

// Context assembly (chapter 1.6): what is sent to the model for one request is put together here, in one
// place, from the parts that other code supplies, in a fixed order:
//
//   tools  ->  system prompt  ->  history  ->  state  ->  the user's latest message  ->  this turn so far
//
// Stable things come first and changing things last, so that the start of every request is the same as the
// start of the one before (a prompt cache can only hit on a start that is the same). The state is not
// written into the history: it is put in front of the user's latest message each time it is asked for.
//
// BuildContext is a pure function: no clock, no random numbers, no global state. The same inputs give the
// same Context, byte for byte. The Context does not depend on the protocol; the provider turns the
// Conversation and the tools into the JSON of its own protocol, as before.
namespace agent {

// A titled piece of text. In the system prompt and in the state, a section is written as "[title]" on a
// line of its own and then the text; a section without a title is the text alone; a section without text
// is left out.
struct Section {
  std::string title;
  std::string text;
};

// How many tokens a text takes. The default is EstimateTokens; tests pass an exact one.
using TokenCounter = std::function<std::size_t(const std::string&)>;

struct ContextInputs {
  std::vector<llm::ToolDefinition> tools;
  std::vector<Section> system;        // joined with blank lines into Conversation::system
  std::vector<llm::Message> history;  // the turns that are stored, before this one
  std::vector<Section> state;         // changes from request to request: put in front of `latest`
  llm::Message latest;                // the user's message that started this turn (role kUser)
  std::vector<llm::Message> turn;     // what this turn has produced so far (the model's calls, the results)
  std::size_t budget_tokens = 0;      // 0: no limit
  TokenCounter count;                 // empty: EstimateTokens
  std::size_t per_message_tokens = 4;  // what a message costs besides its content (roles, separators)
};

struct PartReport {
  std::string name;  // "tools", "system: <title>", "history", "state", "input", "turn"
  std::size_t items = 0;
  std::size_t tokens = 0;
};

struct ContextReport {
  std::vector<PartReport> parts;  // always in the order of the context
  std::size_t total = 0;
  std::size_t budget = 0;
  std::size_t dropped_messages = 0;  // history messages left out to fit the budget
  std::size_t dropped_turns = 0;
};

struct Context {
  llm::Conversation conversation;
  std::vector<llm::ToolDefinition> tools;
  ContextReport report;
};

// Thrown when the context does not fit the budget even without any history. The message says how much
// each part takes.
class ContextError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

// Puts the parts together. When the total is over `budget_tokens`, the oldest turns of the history are
// left out, a whole turn at a time (a turn is a user message with text and everything up to the next
// one), so a tool call is never left without its result and the history still starts with a user
// message. The system prompt, the state, the latest message and the turn are never cut. When something was
// left out, the state says so ("earlier turns not shown: N"). Throws ContextError when it still does not fit.
Context BuildContext(const ContextInputs& inputs);

// A rough count, without a tokenizer: about one token for four letters, digits or spaces, and one for each
// punctuation mark and for each character that is not ASCII. It is not what the service counts: on five
// samples of text it was a fifth to a third too high for an OpenAI-compatible model and a sixth to two fifths
// too low for Claude (chapter 1.6 has the table), and it does not know what a service adds by itself (the
// roles, say). Good enough to see what takes the room and to keep the request away from the window.
std::size_t EstimateTokens(const std::string& text);

// What a session tells the model about itself in the state (the one section every request has). The date is
// the only thing in it that comes from the clock; the caller reads the clock and passes it in, so BuildContext
// stays a pure function and the same time gives the same text.
struct SessionInfo {
  std::string channel;                     // "cli", or the channel of the chat
  bool has_skills = false;                 // the Skill mechanism is on: say what is loaded
  std::vector<std::string> loaded_skills;
  int max_steps = 0;                       // the most model calls for one answer; 0: not said
};

// "2026-11-28 Saturday (UTC+08:00)": the day in the time zone that is `utc_offset` from UTC, its weekday, and
// the offset. A model has no clock and no calendar; without this it guesses the date from what it was trained on.
std::string FormatDate(std::chrono::system_clock::time_point now, std::chrono::minutes utc_offset);

// The offset of the local time zone from UTC at `now` (zero when the system has no time zone data).
std::chrono::minutes LocalUtcOffset(std::chrono::system_clock::time_point now);

// The section "session state": channel, date, and (with Skills) what is loaded and how many steps there are.
Section SessionSection(const SessionInfo& info, std::chrono::system_clock::time_point now, std::chrono::minutes utc_offset);

// The setting context_budget_tokens: a whole number of tokens, 0 (the default) for "what the window leaves" (DefaultBudget).
// Throws llm::Error when the value is not a whole number or is negative.
std::size_t ContextBudgetFromSettings(const config::Values& settings);

// The token budget for the input: what the window leaves after the room for the answer, less a tenth.
std::size_t DefaultBudget(const llm::Provider& provider, const llm::Options& options);

// The context as text, in a fixed layout. For the golden files and `--show-context --verbose`.
std::string FormatContext(const Context& context);

}  // namespace agent
