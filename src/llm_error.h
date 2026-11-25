#pragma once

#include <stdexcept>

namespace llm {

// A message that cannot be understood, a setting that is missing or wrong, or a service that
// answered with an error.
class Error : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

}  // namespace llm
