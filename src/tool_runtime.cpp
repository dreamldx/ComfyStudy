#include "tool_runtime.h"

#include <boost/asio/experimental/awaitable_operators.hpp>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <charconv>
#include <exception>
#include <stdexcept>

#include "llm_error.h"
#include "schema.h"

namespace agent {

namespace {

namespace asio = boost::asio;
using asio::experimental::awaitable_operators::operator||;

ToolResult Error(const std::string& text, Effect effect) {
  ToolResult result{text, true};
  result.effect = effect;
  return result;
}

bool IsCancellation(const boost::system::system_error& e) { return e.code() == asio::error::operation_aborted; }

// `a || b` reports two failures together; a cancellation from outside reaches both, and counts as the
// cancellation of the call as long as the first of them is one.
bool FirstIsCancellation(const asio::multiple_exceptions& e) {
  try {
    std::rethrow_exception(e.first_exception());
  } catch (const boost::system::system_error& first) {
    return IsCancellation(first);
  } catch (...) {
    return false;
  }
}

asio::awaitable<void> Deadline(std::chrono::milliseconds limit) {
  asio::steady_timer timer(co_await asio::this_coro::executor, limit);
  co_await timer.async_wait(asio::use_awaitable);
}

// Calls the tool; whatever it throws becomes an error result, except a cancellation. This is a coroutine of
// its own, and the time limit is put around it (below), so that nothing but a cancellation ever leaves it
// as an exception: `a || b` does not cope well with an operand that throws before its first suspension
// (the timer was left running until it ran out).
asio::awaitable<ToolResult> Guarded(std::shared_ptr<ToolSource> source, std::string name, json::Value arguments, Effect effect) {
  try {
    co_return co_await source->Call(name, arguments);
  } catch (const boost::system::system_error& e) {
    if (IsCancellation(e)) throw;
    co_return Error(std::string("the tool failed: ") + e.what(), effect);
  } catch (const std::exception& e) {
    co_return Error(std::string("the tool failed: ") + e.what(), effect);
  }
}

// Calls the tool and puts a time limit on it. The arguments are taken by value: this coroutine outlives the
// caller's references while `||` runs the call and the timer next to each other.
asio::awaitable<ToolResult> Execute(std::shared_ptr<ToolSource> source, std::string name, json::Value arguments,
                                    std::chrono::milliseconds timeout, Effect effect) {
  if (timeout.count() == 0) co_return co_await Guarded(std::move(source), std::move(name), std::move(arguments), effect);
  try {
    auto outcome = co_await (Guarded(source, name, std::move(arguments), effect) || Deadline(timeout));
    if (outcome.index() == 1) {
      co_return Error("the tool \"" + name + "\" did not finish within " + std::to_string(timeout.count()) + " ms and was stopped", effect);
    }
    co_return std::move(std::get<0>(outcome));
  } catch (const asio::multiple_exceptions& e) {
    // a cancellation from outside reached the call and the timer, and both ended with it
    if (FirstIsCancellation(e)) throw boost::system::system_error(asio::error::operation_aborted);
    try {
      std::rethrow_exception(e.first_exception());
    } catch (const std::exception& first) {
      co_return Error(std::string("the tool failed: ") + first.what(), effect);
    } catch (...) {
      co_return Error("the tool failed", effect);
    }
  }
}

// At most `limit` bytes of `text`, never ending in the middle of a UTF-8 character, and a line that says how
// much was left out.
std::string Cut(const std::string& text, std::size_t limit) {
  std::size_t end = limit;
  while (end > 0 && (static_cast<unsigned char>(text[end]) & 0xC0u) == 0x80u) --end;  // text[end] would start the cut-off part
  return text.substr(0, end) + "\n[truncated: showing " + std::to_string(end) + " of " + std::to_string(text.size()) + " bytes]";
}

// A whole number from a setting, or `fallback` when the setting is missing or empty.
long WholeNumber(const config::Values& settings, const char* name, long fallback, long least) {
  const auto found = settings.find(name);
  if (found == settings.end() || found->second.empty()) return fallback;
  const std::string& text = found->second;
  long value = 0;
  const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
  if (parsed.ec != std::errc() || parsed.ptr != text.data() + text.size() || value < least) {
    throw llm::Error(std::string("setting \"") + name + "\" must be a whole number of at least " + std::to_string(least) + ", not \"" + text +
                     "\"");
  }
  return value;
}

}  // namespace

RuntimeOptions RuntimeOptionsFromSettings(const config::Values& settings) {
  RuntimeOptions options;
  options.timeout = std::chrono::milliseconds(WholeNumber(settings, "tool_timeout_ms", 30000, 0));
  options.max_result_bytes = static_cast<std::size_t>(WholeNumber(settings, "tool_result_max_bytes", 16384, 256));
  return options;
}

ToolRuntime::ToolRuntime(RuntimeOptions options) : options_(options) {}

void ToolRuntime::AddSource(std::shared_ptr<ToolSource> source) {
  for (const ToolSpec& spec : source->Specs()) {
    if (Find(spec.name)) throw std::invalid_argument("two tools are called \"" + spec.name + "\"");
  }
  sources_.push_back(std::move(source));
}

bool ToolRuntime::RemoveSource(const std::shared_ptr<ToolSource>& source) {
  const auto found = std::find(sources_.begin(), sources_.end(), source);
  if (found == sources_.end()) return false;
  sources_.erase(found);
  return true;
}

void ToolRuntime::StartOver() {
  const std::vector<std::shared_ptr<ToolSource>> sources = sources_;  // a source may add or remove sources
  for (const std::shared_ptr<ToolSource>& source : sources) source->StartOver();
}

std::optional<ToolRuntime::Found> ToolRuntime::Find(const std::string& name) const {
  for (const std::shared_ptr<ToolSource>& source : sources_) {
    for (ToolSpec& spec : source->Specs()) {
      if (spec.name == name) return Found{source, std::move(spec)};
    }
  }
  return std::nullopt;
}

std::optional<Effect> ToolRuntime::EffectOf(const std::string& name) const {
  const auto found = Find(name);
  if (!found) return std::nullopt;
  return found->spec.effect;
}

std::vector<llm::ToolDefinition> ToolRuntime::Definitions() const {
  std::vector<llm::ToolDefinition> definitions;
  for (const std::shared_ptr<ToolSource>& source : sources_) {
    for (const ToolSpec& spec : source->Specs()) {
      const bool taken = std::any_of(definitions.begin(), definitions.end(), [&](const llm::ToolDefinition& d) { return d.name == spec.name; });
      if (taken) {
        spdlog::warn("two tools are called \"{}\"; the second one is not offered", spec.name);
        continue;
      }
      if (std::find(warned_.begin(), warned_.end(), spec.name) == warned_.end()) {
        warned_.push_back(spec.name);
        const std::vector<std::string> unchecked = schema::UncheckedKeywords(spec.parameters);
        if (!unchecked.empty()) {
          std::string list;
          for (const std::string& keyword : unchecked) list += (list.empty() ? "" : ", ") + keyword;
          spdlog::warn("the schema of the tool \"{}\" uses keywords that are not checked: {}", spec.name, list);
        }
      }
      definitions.push_back(llm::ToolDefinition{spec.name, spec.description, spec.parameters});
    }
  }
  return definitions;
}

asio::awaitable<ToolResult> ToolRuntime::Run(const llm::ToolCallBlock& call) {
  const auto found = Find(call.name);
  if (!found) {
    std::string known;
    for (const std::shared_ptr<ToolSource>& source : sources_) {
      for (const ToolSpec& spec : source->Specs()) known += (known.empty() ? "" : ", ") + spec.name;
    }
    co_return Error("there is no tool called \"" + call.name + "\"; the tools are: " + known, Effect::kSideEffect);
  }
  const Effect effect = found->spec.effect;

  if (call.arguments.is_null()) co_return Error("the arguments are not valid JSON: " + call.arguments_text, effect);

  if (effect == Effect::kDangerous && !options_.allow_dangerous) {
    co_return Error("the tool \"" + call.name + "\" is classified as dangerous, and dangerous tools are not allowed to run", effect);
  }

  const std::vector<std::string> problems = schema::Validate(found->spec.parameters, call.arguments);
  if (!problems.empty()) {
    std::string text = "the arguments of \"" + call.name + "\" do not fit its schema:";
    for (const std::string& problem : problems) text += "\n- " + problem;
    co_return Error(text, effect);
  }

  const std::chrono::milliseconds timeout = found->spec.timeout.count() > 0 ? found->spec.timeout : options_.timeout;
  ToolResult result = co_await Execute(found->source, call.name, call.arguments, timeout, effect);
  result.effect = effect;
  if (result.text.size() > options_.max_result_bytes) {
    result.text = Cut(result.text, options_.max_result_bytes);
    result.truncated = true;
  }
  co_return result;
}

}  // namespace agent
