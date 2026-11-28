#pragma once

#include <functional>
#include <string>

namespace bot {

// A short code for one failure, 6 characters from an alphabet without the ones that look alike (no 0 and O,
// no 1 and I). The user is told the code, the log has it next to the details, so that "it said ABC234" finds
// the line. `random` gives the numbers (the default is std::random_device); tests pass their own.
std::string MakeErrorId(const std::function<unsigned()>& random = nullptr);

}  // namespace bot
