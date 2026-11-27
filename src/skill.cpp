#include "skill.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <sstream>

namespace agent {

namespace {

namespace fs = std::filesystem;

std::string Trimmed(const std::string& text) {
  std::size_t first = 0;
  std::size_t last = text.size();
  while (first < last && std::isspace(static_cast<unsigned char>(text[first]))) ++first;
  while (last > first && std::isspace(static_cast<unsigned char>(text[last - 1]))) --last;
  return text.substr(first, last - first);
}

// How many characters (not bytes) a UTF-8 text has: the bytes that are not continuation bytes.
std::size_t Characters(const std::string& text) {
  return static_cast<std::size_t>(
      std::count_if(text.begin(), text.end(), [](char c) { return (static_cast<unsigned char>(c) & 0xC0u) != 0x80u; }));
}

std::string Utf8(const fs::path& path) {
  const auto text = path.u8string();
  return std::string(text.begin(), text.end());
}

std::vector<std::string> Lines(const std::string& text) {
  std::vector<std::string> lines;
  std::string line;
  std::istringstream in(text);
  while (std::getline(in, line)) lines.push_back(line);
  return lines;
}

bool ValidNameCharacters(const std::string& name) {
  return std::all_of(name.begin(), name.end(),
                     [](char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-'; });
}

std::string Lower(std::string text) {
  for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return text;
}

}  // namespace

SkillInfo ParseSkill(const std::string& original, const std::string& directory_name) {
  std::string text = original;
  if (text.compare(0, 3, "\xEF\xBB\xBF") == 0) text.erase(0, 3);  // a byte order mark
  std::string unified;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if (text[i] == '\r' && i + 1 < text.size() && text[i + 1] == '\n') continue;  // Windows line ends
    unified += text[i];
  }

  const std::vector<std::string> lines = Lines(unified);
  if (lines.empty() || Trimmed(lines[0]) != "---") {
    throw SkillError("the file must start with a line \"---\": the header (name, description, tools) goes between two such lines");
  }

  std::size_t close = 0;
  for (std::size_t i = 1; i < lines.size(); ++i) {
    if (Trimmed(lines[i]) == "---") {
      close = i;
      break;
    }
  }
  if (close == 0) throw SkillError("the header is not closed: a second line \"---\" is missing");

  SkillInfo skill;
  bool has_name = false;
  bool has_description = false;
  bool has_tools = false;
  for (std::size_t i = 1; i < close; ++i) {
    const std::string line = Trimmed(lines[i]);
    if (line.empty()) continue;
    const std::size_t colon = line.find(':');
    if (colon == std::string::npos) {
      throw SkillError("line " + std::to_string(i + 1) + " of the file (\"" + line + "\") is not of the form \"key: value\"");
    }
    const std::string key = Lower(Trimmed(line.substr(0, colon)));
    const std::string value = Trimmed(line.substr(colon + 1));
    if (key != "name" && key != "description" && key != "tools") {
      skill.warnings.push_back("the key \"" + key + "\" is not known and was left out");
      continue;
    }
    bool& seen = key == "name" ? has_name : key == "description" ? has_description : has_tools;
    if (seen) throw SkillError("the key \"" + key + "\" is given twice");
    seen = true;
    if (key == "name") {
      skill.name = value;
    } else if (key == "description") {
      skill.description = value;
    } else {
      std::istringstream list(value);
      std::string item;
      while (std::getline(list, item, ',')) {
        item = Trimmed(item);
        if (item.empty()) continue;
        if (std::find(skill.tools.begin(), skill.tools.end(), item) == skill.tools.end()) skill.tools.push_back(item);
      }
    }
  }

  if (!has_name) throw SkillError("the header has no \"name\"");
  if (!has_description) throw SkillError("the header has no \"description\"");
  if (!has_tools) throw SkillError("the header has no \"tools\"");

  if (skill.name.empty() || !ValidNameCharacters(skill.name)) {
    throw SkillError("the name \"" + skill.name + "\" is not valid: use lower case letters, digits and hyphens");
  }
  if (skill.name.size() > 40) {
    throw SkillError("the name is too long: at most 40 characters (got " + std::to_string(skill.name.size()) + ")");
  }
  if (skill.name != directory_name) {
    throw SkillError("the name \"" + skill.name + "\" is not the name of its directory \"" + directory_name + "\"");
  }
  if (skill.description.empty()) throw SkillError("the \"description\" is empty: say what the Skill is for");
  const std::size_t characters = Characters(skill.description);
  if (characters > 200) {
    throw SkillError("the description is too long: at most 200 characters (got " + std::to_string(characters) + ")");
  }
  if (skill.tools.empty()) throw SkillError("\"tools\" names no tool: a Skill brings at least one");

  // The body: what follows the header, without the empty lines at its start and the white space at its end.
  std::string body;
  for (std::size_t i = close + 1; i < lines.size(); ++i) body += lines[i] + "\n";
  std::size_t first = 0;
  while (first < body.size() && body[first] == '\n') ++first;
  std::size_t last = body.size();
  while (last > first && std::isspace(static_cast<unsigned char>(body[last - 1]))) --last;
  skill.body = body.substr(first, last - first);
  return skill;
}

SkillCatalog LoadSkills(const fs::path& directory, const std::set<std::string>& available_tools,
                        const std::set<std::string>& reserved_tools) {
  std::error_code error;
  if (!fs::is_directory(directory, error)) throw SkillError("the Skill directory \"" + Utf8(directory) + "\" does not exist");

  std::vector<fs::path> directories;
  for (const fs::directory_entry& entry : fs::directory_iterator(directory)) {
    if (entry.is_directory()) directories.push_back(entry.path());
  }
  std::sort(directories.begin(), directories.end(),
            [](const fs::path& a, const fs::path& b) { return Utf8(a.filename()) < Utf8(b.filename()); });

  SkillCatalog catalog;
  for (const fs::path& path : directories) {
    const std::string name = Utf8(path.filename());
    std::ifstream in(path / "SKILL.md", std::ios::binary);
    if (!in) {
      catalog.problems.push_back(name + ": there is no SKILL.md in this directory, so it is not a Skill");
      continue;
    }
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    try {
      SkillInfo skill = ParseSkill(text, name);
      for (const std::string& tool : skill.tools) {
        if (reserved_tools.count(tool) != 0) {
          throw SkillError("the tool \"" + tool + "\" belongs to the Skill mechanism itself and cannot be brought by a Skill");
        }
        if (available_tools.count(tool) == 0) throw SkillError("the tool \"" + tool + "\" does not exist");
      }
      for (const std::string& warning : skill.warnings) catalog.problems.push_back(name + ": " + warning);
      catalog.skills.push_back(std::move(skill));
    } catch (const SkillError& e) {
      catalog.problems.push_back(name + ": " + e.what() + " (the Skill was skipped)");
    }
  }
  return catalog;
}

}  // namespace agent
