#include "skill_setup.h"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <set>
#include <stdexcept>

#include "skill_tools.h"

namespace agent {

SkillSetup LoadSkillSetup(const config::Values& settings) {
  SkillSetup setup;
  setup.library = std::make_shared<NativeTools>();
  AddToyTools(*setup.library);
  const std::vector<std::string> names = setup.library->Names();
  const auto found = settings.find("skills_dir");
  const std::filesystem::path directory = found == settings.end() || found->second.empty() ? "skills" : found->second;
  setup.catalog = LoadSkills(directory, std::set<std::string>(names.begin(), names.end()), {SkillTools::kLoad, SkillTools::kUnload});
  for (const std::string& problem : setup.catalog.problems) spdlog::warn("Skill {}", problem);
  if (setup.catalog.skills.empty()) throw std::runtime_error("there is no usable Skill in \"" + directory.string() + "\"");
  return setup;
}

}  // namespace agent
