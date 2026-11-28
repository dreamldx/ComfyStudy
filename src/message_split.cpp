#include "message_split.h"

#include <algorithm>

namespace bot {

namespace {

bool IsContinuation(char c) { return (static_cast<unsigned char>(c) & 0xC0u) == 0x80u; }

std::size_t Chars(const std::string& text) {
  return static_cast<std::size_t>(std::count_if(text.begin(), text.end(), [](char c) { return !IsContinuation(c); }));
}

// The byte offset after the first `count` characters of `text` (the end of it when it has fewer).
std::size_t ByteOffset(const std::string& text, std::size_t count) {
  std::size_t seen = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (IsContinuation(text[i])) continue;
    if (seen == count) return i;
    ++seen;
  }
  return text.size();
}

bool Blank(const std::string& text) {
  return std::all_of(text.begin(), text.end(), [](char c) { return c == ' ' || c == '\n' || c == '\r' || c == '\t'; });
}

// Is a code fence open at the end of `text`? (A line that starts with three backticks opens or closes one.)
// `language` is what follows the backticks of the line that opened it.
bool FenceOpen(const std::string& text, std::string& language) {
  bool open = false;
  std::size_t line_start = 0;
  while (line_start <= text.size()) {
    std::size_t line_end = text.find('\n', line_start);
    if (line_end == std::string::npos) line_end = text.size();
    std::size_t at = line_start;
    while (at < line_end && text[at] == ' ') ++at;
    if (text.compare(at, 3, "```") == 0 && at + 3 <= line_end) {
      open = !open;
      if (open) {
        language = text.substr(at + 3, line_end - at - 3);
        while (!language.empty() && (language.back() == '\r' || language.back() == ' ')) language.pop_back();
      }
    }
    if (line_end == text.size()) break;
    line_start = line_end + 1;
  }
  return open;
}

// Where to end the message that starts `rest`: after a blank line, a line end or a space in the last three
// quarters of the first `room_bytes`, else at `room_bytes`.
std::size_t BreakAt(const std::string& rest, std::size_t room_bytes) {
  const std::string window = rest.substr(0, room_bytes);
  const std::size_t earliest = room_bytes / 4;
  const std::size_t paragraph = window.rfind("\n\n");
  if (paragraph != std::string::npos && paragraph >= earliest) return paragraph + 2;
  const std::size_t line = window.rfind('\n');
  if (line != std::string::npos && line >= earliest) return line + 1;
  const std::size_t space = window.rfind(' ');
  if (space != std::string::npos && space >= earliest) return space + 1;
  return room_bytes;
}

std::string TrimmedRight(std::string text) {
  while (!text.empty() && (text.back() == ' ' || text.back() == '\n' || text.back() == '\r' || text.back() == '\t')) text.pop_back();
  return text;
}

}  // namespace

std::vector<std::string> SplitMessage(const std::string& text, std::size_t limit) {
  std::vector<std::string> messages;
  const std::string closer = "\n```";
  std::string rest = text;
  std::string prefix;  // the fence to open again at the start of this message
  while (true) {
    const std::string whole = prefix + rest;
    if (Chars(whole) <= limit) {
      if (!Blank(whole)) messages.push_back(whole);
      break;
    }
    // Leave room for the fence that is opened again and for the one that may have to be closed.
    const std::size_t reserved = Chars(prefix) + Chars(closer);
    const std::size_t room_chars = limit > reserved + 1 ? limit - reserved : 1;
    const std::size_t room_bytes = ByteOffset(rest, room_chars);
    const std::size_t cut = std::max<std::size_t>(BreakAt(rest, room_bytes), 1);

    std::string head = prefix + TrimmedRight(rest.substr(0, cut));
    std::size_t skip = cut;
    while (skip < rest.size() && (rest[skip] == '\n' || rest[skip] == '\r')) ++skip;  // not the spaces: they may be indentation
    std::string tail = rest.substr(skip);

    std::string language;
    if (FenceOpen(head, language)) {
      head += closer;
      prefix = "```" + language + "\n";
    } else {
      prefix.clear();
    }
    if (!Blank(head)) messages.push_back(head);
    rest = tail;
    if (rest.empty()) break;
  }
  return messages;
}

}  // namespace bot
