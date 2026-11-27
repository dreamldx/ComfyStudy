#include "tool.h"

namespace agent {

const char* EffectName(Effect effect) {
  switch (effect) {
    case Effect::kReadOnly:
      return "read-only";
    case Effect::kSideEffect:
      return "side-effect";
    case Effect::kDangerous:
      return "dangerous";
  }
  return "side-effect";
}

}  // namespace agent
