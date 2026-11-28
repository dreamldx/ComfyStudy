#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace bot {

// Discord takes at most 2000 characters in a message. Cuts `text` into messages of at most `limit`
// characters (characters, not bytes). It breaks at a blank line when there is one in the last three quarters
// of the room, else at the end of a line, else at a space, else wherever it has to; blank space at the cut is
// dropped. A code fence that the cut falls inside is closed at the end of one message and opened again, with
// its language, at the start of the next, so that every message shows its code as code. Text that is only
// blank space gives no message.
std::vector<std::string> SplitMessage(const std::string& text, std::size_t limit = 2000);

}  // namespace bot
