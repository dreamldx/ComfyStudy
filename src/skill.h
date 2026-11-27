#pragma once

#include <filesystem>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

// Skills (chapter 1.5). A Skill is a directory with a file SKILL.md: a short header that says what the
// Skill is for and which tools it brings, and a body that tells the model how to use them. The model
// first sees only the names and the tasks of the Skills; when it loads one (skill_tools.h) it gets the
// body and the tools.
//
//   skills/calculator/SKILL.md
//   ---
//   name: calculator                the name of the directory: 1 to 40 of a-z, 0-9 and -
//   description: exact arithmetic   what it is for, 1 to 200 characters; the model reads this to choose
//   tools: add                      the tools it brings, separated by commas (at least one)
//   ---
//   Use add for sums. ...           the body: handed to the model as it is when the Skill is loaded
//
// The header is a very small part of YAML (one "key: value" per line); the file format is the business
// of quest llm-4.1.
namespace agent {

class SkillError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

struct SkillInfo {
  std::string name;
  std::string description;
  std::vector<std::string> tools;     // without repeats, in the order written
  std::string body;
  std::vector<std::string> warnings;  // what was left out (a key that is not known)
};

// The Skill in `text`, the content of the SKILL.md of the directory called `directory_name`. Throws
// SkillError, whose message says what is wrong with the file (the key, or the line of the header).
SkillInfo ParseSkill(const std::string& text, const std::string& directory_name);

struct SkillCatalog {
  std::vector<SkillInfo> skills;      // sorted by name
  std::vector<std::string> problems;  // one sentence for each Skill that was skipped, or that has a warning
};

// Reads the Skills in the subdirectories of `directory`. A Skill that cannot be used (no SKILL.md, a file
// that does not parse, a tool that is not in `available_tools`, a tool whose name is in `reserved_tools`)
// is skipped and told in `problems`; the others are not affected. Throws SkillError when `directory` is
// not a directory.
SkillCatalog LoadSkills(const std::filesystem::path& directory, const std::set<std::string>& available_tools,
                        const std::set<std::string>& reserved_tools);

}  // namespace agent
