#include "skill.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>

namespace {

namespace fs = std::filesystem;

bool Contains(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

// The message of the SkillError that `text` makes (empty when there is none).
std::string ErrorOf(const std::string& text, const std::string& directory = "calc") {
  try {
    agent::ParseSkill(text, directory);
  } catch (const agent::SkillError& e) {
    return e.what();
  }
  return "";
}

const std::string kGood = "---\nname: calc\ndescription: exact arithmetic\ntools: add\n---\nUse add for sums.\n";

// A directory that is removed when the test ends.
class TempDir {
 public:
  TempDir() {
    path_ = fs::temp_directory_path() / ("comfystudy-skills-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
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

const std::set<std::string> kTools = {"add", "sleep", "repeat"};
const std::set<std::string> kReserved = {"load_skill", "unload_skill"};

std::string Header(const std::string& name, const std::string& tools = "add") {
  return "---\nname: " + name + "\ndescription: does " + name + "\ntools: " + tools + "\n---\nbody of " + name + "\n";
}

}  // namespace

TEST(ParseSkill, TheHeaderGivesTheNameTheTaskAndTheToolsAndTheRestIsTheBody) {
  const agent::SkillInfo skill = agent::ParseSkill("---\nname: calc\ndescription: exact arithmetic\ntools: add, sleep\n---\nUse add.\n\nSecond paragraph.\n", "calc");
  EXPECT_EQ(skill.name, "calc");
  EXPECT_EQ(skill.description, "exact arithmetic");
  EXPECT_EQ(skill.tools, (std::vector<std::string>{"add", "sleep"}));
  EXPECT_EQ(skill.body, "Use add.\n\nSecond paragraph.");
  EXPECT_TRUE(skill.warnings.empty());
}

TEST(ParseSkill, WindowsLineEndingsAndAByteOrderMarkAreAccepted) {
  const agent::SkillInfo skill = agent::ParseSkill("\xEF\xBB\xBF---\r\nname: calc\r\ndescription: exact arithmetic\r\ntools: add\r\n---\r\nLine one.\r\nLine two.\r\n", "calc");
  EXPECT_EQ(skill.name, "calc");
  EXPECT_EQ(skill.tools, std::vector<std::string>{"add"});
  EXPECT_EQ(skill.body, "Line one.\nLine two.");
}

TEST(ParseSkill, SpacesAroundTheKeyTheValueAndTheToolsAreIgnoredAndTheKeysMayComeInAnyOrder) {
  const agent::SkillInfo skill = agent::ParseSkill("---\ntools:  add ,sleep , add\ndescription:   exact arithmetic  \nname :calc\n---\nx", "calc");
  EXPECT_EQ(skill.name, "calc");
  EXPECT_EQ(skill.description, "exact arithmetic");
  EXPECT_EQ(skill.tools, (std::vector<std::string>{"add", "sleep"}));  // a tool named twice counts once
}

TEST(ParseSkill, AKeyThatIsNotKnownIsLeftOutAndReported) {
  const agent::SkillInfo skill = agent::ParseSkill("---\nname: calc\ndescription: d\ntools: add\nversion: 2\n---\nx", "calc");
  ASSERT_EQ(skill.warnings.size(), 1u);
  EXPECT_TRUE(Contains(skill.warnings[0], "version"));
}

TEST(ParseSkill, ABodyMayBeEmpty) {
  EXPECT_EQ(agent::ParseSkill("---\nname: calc\ndescription: d\ntools: add\n---\n", "calc").body, "");
}

TEST(ParseSkill, EachMissingKeyIsNamedInTheError) {
  EXPECT_TRUE(Contains(ErrorOf("---\ndescription: d\ntools: add\n---\n"), "\"name\""));
  EXPECT_TRUE(Contains(ErrorOf("---\nname: calc\ntools: add\n---\n"), "\"description\""));
  EXPECT_TRUE(Contains(ErrorOf("---\nname: calc\ndescription: d\n---\n"), "\"tools\""));
}

TEST(ParseSkill, TheNameHasToBeTheNameOfTheDirectory) {
  const std::string error = ErrorOf(kGood, "other");
  EXPECT_TRUE(Contains(error, "\"calc\"")) << error;
  EXPECT_TRUE(Contains(error, "\"other\"")) << error;
}

TEST(ParseSkill, ANameIsLowerCaseLettersDigitsAndHyphensOfAtMost40Characters) {
  EXPECT_TRUE(Contains(ErrorOf(Header("Calc"), "Calc"), "name"));
  EXPECT_TRUE(Contains(ErrorOf(Header("my skill"), "my skill"), "name"));
  EXPECT_TRUE(Contains(ErrorOf(Header("a_b"), "a_b"), "name"));
  const std::string long_name(41, 'a');
  EXPECT_TRUE(Contains(ErrorOf(Header(long_name), long_name), "at most 40"));
  const std::string longest(40, 'a');
  EXPECT_EQ(ErrorOf(Header(longest), longest), "");
  EXPECT_EQ(ErrorOf(Header("a-1"), "a-1"), "");
}

TEST(ParseSkill, TheTaskIsAtMost200CharactersCountedAsCharactersNotBytes) {
  std::string chinese;
  for (int i = 0; i < 200; ++i) chinese += "你";  // 600 bytes
  EXPECT_EQ(ErrorOf("---\nname: calc\ndescription: " + chinese + "\ntools: add\n---\n"), "");
  EXPECT_TRUE(Contains(ErrorOf("---\nname: calc\ndescription: " + chinese + "a\ntools: add\n---\n"), "at most 200"));
  EXPECT_TRUE(Contains(ErrorOf("---\nname: calc\ndescription:\ntools: add\n---\n"), "description"));
}

TEST(ParseSkill, AtLeastOneToolIsNeeded) {
  EXPECT_TRUE(Contains(ErrorOf("---\nname: calc\ndescription: d\ntools:  , \n---\n"), "tools"));
}

TEST(ParseSkill, AFileWithoutAHeaderOrWithAHeaderThatIsNotClosedIsAnError) {
  EXPECT_TRUE(Contains(ErrorOf("just text"), "---"));
  EXPECT_TRUE(Contains(ErrorOf("---\nname: calc\ndescription: d\ntools: add\n"), "not closed"));
  EXPECT_TRUE(Contains(ErrorOf(""), "---"));
}

TEST(ParseSkill, ALineOfTheHeaderWithoutAColonIsAnErrorThatSaysWhichLine) {
  const std::string error = ErrorOf("---\nname: calc\nnonsense\ndescription: d\ntools: add\n---\n");
  EXPECT_TRUE(Contains(error, "line 3")) << error;
}

TEST(LoadSkills, EveryDirectoryWithASkillFileIsLoadedAndTheListIsSortedByName) {
  TempDir dir;
  dir.Write("timer/SKILL.md", Header("timer", "sleep"));
  dir.Write("calc/SKILL.md", Header("calc"));
  const agent::SkillCatalog catalog = agent::LoadSkills(dir.path(), kTools, kReserved);
  ASSERT_EQ(catalog.skills.size(), 2u);
  EXPECT_EQ(catalog.skills[0].name, "calc");
  EXPECT_EQ(catalog.skills[1].name, "timer");
  EXPECT_TRUE(catalog.problems.empty());
}

TEST(LoadSkills, ABadSkillIsSkippedWithAProblemAndTheOthersStay) {
  TempDir dir;
  dir.Write("calc/SKILL.md", Header("calc"));
  dir.Write("broken/SKILL.md", "no header at all");
  dir.Write("empty/readme.txt", "no skill file here");
  dir.Write("stray.txt", "a file next to the skills is not a skill");
  const agent::SkillCatalog catalog = agent::LoadSkills(dir.path(), kTools, kReserved);
  ASSERT_EQ(catalog.skills.size(), 1u);
  EXPECT_EQ(catalog.skills[0].name, "calc");
  ASSERT_EQ(catalog.problems.size(), 2u);
  EXPECT_TRUE(Contains(catalog.problems[0], "broken"));
  EXPECT_TRUE(Contains(catalog.problems[1], "empty"));
  EXPECT_TRUE(Contains(catalog.problems[1], "SKILL.md"));
}

TEST(LoadSkills, AToolThatDoesNotExistMakesTheSkillUnusableAndTheProblemNamesTheTool) {
  TempDir dir;
  dir.Write("calc/SKILL.md", Header("calc", "add, multiply"));
  const agent::SkillCatalog catalog = agent::LoadSkills(dir.path(), kTools, kReserved);
  EXPECT_TRUE(catalog.skills.empty());
  ASSERT_EQ(catalog.problems.size(), 1u);
  EXPECT_TRUE(Contains(catalog.problems[0], "multiply"));
  EXPECT_TRUE(Contains(catalog.problems[0], "calc"));
}

TEST(LoadSkills, ASkillCannotClaimAToolOfTheSkillMechanismItself) {
  TempDir dir;
  dir.Write("calc/SKILL.md", Header("calc", "load_skill"));
  const agent::SkillCatalog catalog = agent::LoadSkills(dir.path(), kTools, kReserved);
  EXPECT_TRUE(catalog.skills.empty());
  ASSERT_EQ(catalog.problems.size(), 1u);
  EXPECT_TRUE(Contains(catalog.problems[0], "load_skill"));
}

TEST(LoadSkills, TheWarningsOfAParsedSkillBecomeProblemsWithoutSkippingIt) {
  TempDir dir;
  dir.Write("calc/SKILL.md", "---\nname: calc\ndescription: d\ntools: add\nversion: 2\n---\nx");
  const agent::SkillCatalog catalog = agent::LoadSkills(dir.path(), kTools, kReserved);
  EXPECT_EQ(catalog.skills.size(), 1u);
  ASSERT_EQ(catalog.problems.size(), 1u);
  EXPECT_TRUE(Contains(catalog.problems[0], "version"));
}

TEST(LoadSkills, ADirectoryThatDoesNotExistIsAnError) {
  TempDir dir;
  EXPECT_THROW(agent::LoadSkills(dir.path() / "nothing-here", kTools, kReserved), agent::SkillError);
}
