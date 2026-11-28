#include "skill_setup.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

namespace fs = std::filesystem;

class TempDir {
 public:
  TempDir() {
    path_ = fs::temp_directory_path() / ("comfystudy-setup-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(path_);
  }
  ~TempDir() {
    std::error_code ignored;
    fs::remove_all(path_, ignored);
  }
  const fs::path& path() const { return path_; }
  void Write(const std::string& relative, const std::string& content) const {
    fs::create_directories((path_ / relative).parent_path());
    std::ofstream(path_ / relative, std::ios::binary) << content;
  }

 private:
  fs::path path_;
};

const std::string kSkill = "---\nname: calc\ndescription: exact arithmetic\ntools: add\n---\nUse add.\n";

}  // namespace

TEST(LoadSkillSetup, HasTheToyToolsInTheLibraryAndTheSkillsOfTheDirectoryInTheSetting) {
  TempDir dir;
  dir.Write("calc/SKILL.md", kSkill);
  const agent::SkillSetup setup = agent::LoadSkillSetup({{"skills_dir", dir.path().string()}});
  EXPECT_TRUE(setup.library->Has("add"));
  EXPECT_TRUE(setup.library->Has("shutdown"));
  ASSERT_EQ(setup.catalog.skills.size(), 1u);
  EXPECT_EQ(setup.catalog.skills[0].name, "calc");
}

TEST(LoadSkillSetup, ADirectoryWithoutAUsableSkillIsAnErrorThatNamesIt) {
  TempDir dir;
  dir.Write("broken/SKILL.md", "no header");
  try {
    agent::LoadSkillSetup({{"skills_dir", dir.path().string()}});
    FAIL() << "should have thrown";
  } catch (const std::runtime_error& e) {
    EXPECT_NE(std::string(e.what()).find(dir.path().string()), std::string::npos) << e.what();
  }
}

TEST(LoadSkillSetup, ADirectoryThatDoesNotExistIsAnError) {
  TempDir dir;
  EXPECT_THROW(agent::LoadSkillSetup({{"skills_dir", (dir.path() / "nothing").string()}}), agent::SkillError);
}
