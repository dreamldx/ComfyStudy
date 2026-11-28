#include "context.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include <charconv>

#include "json.h"
#include "llm_error.h"

namespace agent {

namespace {

std::string Render(const Section& section) { return section.title.empty() ? section.text : "[" + section.title + "]\n" + section.text; }

// The sections that have text, one after the other with a blank line between them.
std::string RenderAll(const std::vector<Section>& sections) {
  std::string out;
  for (const Section& section : sections) {
    if (section.text.empty()) continue;
    out += (out.empty() ? "" : "\n\n") + Render(section);
  }
  return out;
}

struct Counter {
  const ContextInputs& inputs;
  std::size_t operator()(const std::string& text) const { return inputs.count ? inputs.count(text) : EstimateTokens(text); }

  std::size_t Block(const llm::Block& block) const {
    if (const auto* text = std::get_if<llm::TextBlock>(&block)) return (*this)(text->text);
    // The whole payload, signature included: it is sent back, and the services count it (a Claude request with
    // earlier answers that thought was about three times what the words alone came to).
    if (const auto* reasoning = std::get_if<llm::ReasoningBlock>(&block)) return (*this)(json::Dump(reasoning->payload));
    if (const auto* call = std::get_if<llm::ToolCallBlock>(&block)) {
      return (*this)(call->name) + (*this)(call->arguments_text.empty() ? json::Dump(call->arguments) : call->arguments_text);
    }
    const auto& result = std::get<llm::ToolResultBlock>(block);
    return (*this)(result.text);
  }
  std::size_t Message(const llm::Message& message) const {
    std::size_t tokens = inputs.per_message_tokens;
    for (const llm::Block& block : message.blocks) tokens += Block(block);
    return tokens;
  }
  std::size_t Messages(const std::vector<llm::Message>& messages, std::size_t first = 0) const {
    std::size_t tokens = 0;
    for (std::size_t i = first; i < messages.size(); ++i) tokens += Message(messages[i]);
    return tokens;
  }
  std::size_t Tools(const std::vector<llm::ToolDefinition>& tools) const {
    std::size_t tokens = 0;
    for (const llm::ToolDefinition& tool : tools) tokens += (*this)(tool.name) + (*this)(tool.description) + (*this)(json::Dump(tool.parameters));
    return tokens;
  }
};

// A user message that starts a turn: one without tool results (a message with results continues the turn
// of the call it answers, even when it also carries a new message of the user's).
bool StartsTurn(const llm::Message& message) {
  if (message.role != llm::Role::kUser) return false;
  return std::none_of(message.blocks.begin(), message.blocks.end(),
                      [](const llm::Block& block) { return std::holds_alternative<llm::ToolResultBlock>(block); });
}

std::string NoteAbout(std::size_t turns) { return "earlier turns not shown: " + std::to_string(turns); }

const char* RoleName(llm::Role role) { return role == llm::Role::kUser ? "user" : "assistant"; }

// The text of a block, each line marked so that a line that looks like a heading cannot be taken for one.
std::string Quoted(const std::string& text) {
  std::string out;
  std::istringstream in(text);
  std::string line;
  bool any = false;
  while (std::getline(in, line)) {
    out += "    | " + line + "\n";
    any = true;
  }
  if (!any) out += "    |\n";
  return out;
}

}  // namespace

std::size_t EstimateTokens(const std::string& text) {
  std::size_t plain = 0;  // letters, digits and white space: about four to a token
  std::size_t marks = 0;  // ASCII punctuation: about one token each (JSON, code and tool definitions are full of it)
  std::size_t others = 0;  // every other character (Chinese, accents, symbols): about one token each
  for (const char c : text) {
    const auto byte = static_cast<unsigned char>(c);
    if (byte >= 0x80) {
      if ((byte & 0xC0u) != 0x80u) ++others;  // the first byte of a character
    } else if (std::isalnum(byte) != 0 || std::isspace(byte) != 0) {
      ++plain;
    } else {
      ++marks;
    }
  }
  return (plain + 3) / 4 + marks + others;
}

std::size_t ContextBudgetFromSettings(const config::Values& settings) {
  const auto found = settings.find("context_budget_tokens");
  if (found == settings.end() || found->second.empty()) return 0;
  const std::string& text = found->second;
  std::size_t value = 0;
  const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
  if (parsed.ec != std::errc() || parsed.ptr != text.data() + text.size()) {
    throw llm::Error("setting \"context_budget_tokens\" must be a whole number of tokens (0 for the window of the model), not \"" + text + "\"");
  }
  return value;
}

std::size_t DefaultBudget(const llm::Provider& provider, const llm::Options& options) {
  const long window = provider.capabilities.context_window;
  const long room = window - options.max_tokens;
  if (room <= 0) return 1;
  return static_cast<std::size_t>(room - room / 10);
}

Context BuildContext(const ContextInputs& inputs) {
  const Counter count{inputs};

  // The turns of the history: where each starts. What comes before the first start (a history that does not
  // begin with a user message) belongs to the first turn.
  std::vector<std::size_t> starts;
  for (std::size_t i = 0; i < inputs.history.size(); ++i) {
    if (StartsTurn(inputs.history[i])) starts.push_back(i);
  }
  if (!inputs.history.empty() && (starts.empty() || starts.front() != 0)) starts.insert(starts.begin(), 0);

  std::vector<std::size_t> history_tokens;
  for (const llm::Message& message : inputs.history) history_tokens.push_back(count.Message(message));

  std::vector<Section> system_parts;
  for (const Section& section : inputs.system) {
    if (!section.text.empty()) system_parts.push_back(section);
  }

  const std::size_t tools_tokens = count.Tools(inputs.tools);
  std::size_t system_tokens = 0;
  for (const Section& section : system_parts) system_tokens += count(Render(section));
  const std::size_t input_tokens = count.Message(inputs.latest);
  const std::size_t turn_tokens = count.Messages(inputs.turn);

  // How much the state takes when `dropped` turns are left out (the note about it is part of the state).
  const auto state_for = [&](std::size_t dropped) {
    std::vector<Section> sections = inputs.state;
    if (dropped > 0) sections.push_back({"context", NoteAbout(dropped)});
    return RenderAll(sections);
  };

  std::size_t dropped_turns = 0;
  std::size_t first_kept = 0;  // index in the history of the first message that stays
  const auto total_for = [&](std::size_t kept_from, std::size_t dropped) {
    std::size_t history = 0;
    for (std::size_t i = kept_from; i < history_tokens.size(); ++i) history += history_tokens[i];
    return tools_tokens + system_tokens + history + count(state_for(dropped)) + input_tokens + turn_tokens;
  };

  if (inputs.budget_tokens > 0) {
    while (total_for(first_kept, dropped_turns) > inputs.budget_tokens && dropped_turns < starts.size()) {
      ++dropped_turns;
      first_kept = dropped_turns < starts.size() ? starts[dropped_turns] : inputs.history.size();
    }
  }

  Context context;
  context.tools = inputs.tools;
  context.conversation.system = RenderAll(system_parts);

  const std::string state_text = state_for(dropped_turns);
  for (std::size_t i = first_kept; i < inputs.history.size(); ++i) context.conversation.messages.push_back(inputs.history[i]);
  llm::Message latest = inputs.latest;
  if (!state_text.empty()) latest.blocks.insert(latest.blocks.begin(), llm::TextBlock{state_text});
  context.conversation.messages.push_back(std::move(latest));
  for (const llm::Message& message : inputs.turn) context.conversation.messages.push_back(message);

  ContextReport& report = context.report;
  report.budget = inputs.budget_tokens;
  report.dropped_turns = dropped_turns;
  report.dropped_messages = first_kept;
  report.parts.push_back({"tools", inputs.tools.size(), tools_tokens});
  for (const Section& section : system_parts) {
    report.parts.push_back({"system: " + (section.title.empty() ? std::string("instructions") : section.title), 1, count(Render(section))});
  }
  std::size_t kept_tokens = 0;
  for (std::size_t i = first_kept; i < history_tokens.size(); ++i) kept_tokens += history_tokens[i];
  report.parts.push_back({"history", inputs.history.size() - first_kept, kept_tokens});
  std::size_t state_items = 0;
  for (const Section& section : inputs.state) state_items += section.text.empty() ? 0 : 1;
  state_items += dropped_turns > 0 ? 1 : 0;
  report.parts.push_back({"state", state_items, count(state_text)});
  report.parts.push_back({"input", 1, input_tokens});
  report.parts.push_back({"turn", inputs.turn.size(), turn_tokens});
  for (const PartReport& part : report.parts) report.total += part.tokens;

  if (inputs.budget_tokens > 0 && report.total > inputs.budget_tokens) {
    std::string parts;
    for (const PartReport& part : report.parts) parts += (parts.empty() ? "" : ", ") + part.name + " " + std::to_string(part.tokens);
    throw ContextError("the context needs about " + std::to_string(report.total) + " tokens, more than the budget of " +
                       std::to_string(inputs.budget_tokens) + " even without the history (" + parts + ")");
  }
  return context;
}

std::string FormatContext(const Context& context) {
  std::ostringstream out;
  out << "== tools (" << context.tools.size() << ") ==\n";
  for (const llm::ToolDefinition& tool : context.tools) {
    out << tool.name << ": " << tool.description << "\n";
    out << Quoted(json::Dump(tool.parameters));
  }
  out << "== system ==\n";
  out << (context.conversation.system.empty() ? "(none)\n" : Quoted(context.conversation.system));
  out << "== messages (" << context.conversation.messages.size() << ") ==\n";
  std::size_t number = 0;
  for (const llm::Message& message : context.conversation.messages) {
    out << "[" << ++number << "] " << RoleName(message.role) << "\n";
    for (const llm::Block& block : message.blocks) {
      if (const auto* text = std::get_if<llm::TextBlock>(&block)) {
        out << "  text\n" << Quoted(text->text);
      } else if (const auto* reasoning = std::get_if<llm::ReasoningBlock>(&block)) {
        out << "  reasoning " << (reasoning->origin == llm::Protocol::kAnthropic ? "anthropic" : "openai") << "\n" << Quoted(json::Dump(reasoning->payload));
      } else if (const auto* call = std::get_if<llm::ToolCallBlock>(&block)) {
        out << "  tool_call id=" << call->id << " name=" << call->name << "\n"
            << Quoted(call->arguments_text.empty() ? json::Dump(call->arguments) : call->arguments_text);
      } else {
        const auto& result = std::get<llm::ToolResultBlock>(block);
        out << "  tool_result id=" << result.call_id << " error=" << (result.is_error ? "true" : "false") << "\n" << Quoted(result.text);
      }
    }
  }
  out << "== report ==\n";
  for (const PartReport& part : context.report.parts) {
    out << "  " << part.name << std::string(part.name.size() < 24 ? 24 - part.name.size() : 1, ' ') << part.items << " items, " << part.tokens << " tokens\n";
  }
  out << "  total " << context.report.total << " tokens";
  if (context.report.budget > 0) out << " of " << context.report.budget;
  out << "; dropped " << context.report.dropped_messages << " messages (" << context.report.dropped_turns << " turns)\n";
  return out.str();
}

}  // namespace agent
