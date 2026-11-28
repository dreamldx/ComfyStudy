#pragma once

#include <memory>

#include "config.h"
#include "native_tools.h"
#include "skill.h"

namespace agent {

// What the Skill mechanism needs from the settings (chapters 1.5 and 1.7): the toy tools in a library, and the
// Skills of the directory in the setting skills_dir (default "skills") that can be used. Skills that cannot be
// used are skipped with a warning in the log. Throws std::runtime_error naming the directory when there is not
// one usable Skill (and SkillError when the directory does not exist).
struct SkillSetup {
  std::shared_ptr<NativeTools> library;
  SkillCatalog catalog;
};

SkillSetup LoadSkillSetup(const config::Values& settings);

}  // namespace agent
