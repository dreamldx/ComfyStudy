#include "error_id.h"

#include <random>

namespace bot {

std::string MakeErrorId(const std::function<unsigned()>& random) {
  static const char kAlphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";  // 32 characters
  constexpr std::size_t kLength = 6;
  std::random_device device;
  std::string id;
  for (std::size_t i = 0; i < kLength; ++i) {
    const unsigned number = random ? random() : device();
    id += kAlphabet[number % (sizeof(kAlphabet) - 1)];
  }
  return id;
}

}  // namespace bot
