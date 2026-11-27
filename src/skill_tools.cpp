#include "skill_tools.h"

#include <algorithm>
#include <stdexcept>

namespace agent {

namespace {

std::string Joined(const std::vector<std::string>& items, const char* separator = ", ") {
  std::string text;
  for (const std::string& item : items) text += (text.empty() ? "" : separator) + item;
  return text;
}

ToolResult Failure(const std::string& text) {
  ToolResult result{text, true};
  result.effect = Effect::kSideEffect;
  return result;
}

}  // namespace

SkillTools::SkillTools(SkillCatalog catalog, std::shared_ptr<NativeTools> library, ToolRuntime& runtime)
    : catalog_(std::move(catalog)), library_(std::move(library)), runtime_(runtime) {
  for (const SkillInfo& skill : catalog_.skills) {
    for (const std::string& tool : skill.tools) {
      if (!library_->Has(tool)) throw std::invalid_argument("the Skill \"" + skill.name + "\" names the tool \"" + tool + "\", which does not exist");
    }
  }
}

const SkillInfo* SkillTools::FindSkill(const std::string& name) const {
  for (const SkillInfo& skill : catalog_.skills) {
    if (skill.name == name) return &skill;
  }
  return nullptr;
}

bool SkillTools::IsLoaded(const std::string& name) const { return std::find(loaded_.begin(), loaded_.end(), name) != loaded_.end(); }

void SkillTools::Attach(const SkillInfo& skill) {
  std::vector<std::string> added;
  try {
    for (const std::string& tool : skill.tools) {
      Count& count = counts_[tool];
      if (count.skills == 0) {
        auto source = library_->Subset({tool});
        runtime_.AddSource(source);  // throws when another source has a tool of this name
        count.source = std::move(source);
      }
      ++count.skills;
      added.push_back(tool);
    }
  } catch (...) {
    // take back what this Skill had added so far, so that a Skill is loaded whole or not at all
    for (const std::string& tool : added) {
      Count& count = counts_[tool];
      if (--count.skills == 0) {
        runtime_.RemoveSource(count.source);
        counts_.erase(tool);
      }
    }
    const auto failed = counts_.find(skill.tools[added.size()]);
    if (failed != counts_.end() && failed->second.skills == 0) counts_.erase(failed);
    throw;
  }
}

std::vector<std::string> SkillTools::Detach(const SkillInfo& skill) {
  std::vector<std::string> removed;
  for (const std::string& tool : skill.tools) {
    const auto found = counts_.find(tool);
    if (found == counts_.end()) continue;
    if (--found->second.skills == 0) {
      runtime_.RemoveSource(found->second.source);
      counts_.erase(found);
      removed.push_back(tool);
    }
  }
  return removed;
}

void SkillTools::Restore(const std::vector<std::string>& wanted) {
  try {
    const std::vector<std::string> now = loaded_;
    for (auto it = now.rbegin(); it != now.rend(); ++it) {  // the last loaded goes first
      if (std::find(wanted.begin(), wanted.end(), *it) != wanted.end()) continue;
      if (const SkillInfo* skill = FindSkill(*it)) Detach(*skill);
      loaded_.erase(std::find(loaded_.begin(), loaded_.end(), *it));
    }
    for (const std::string& name : wanted) {
      if (IsLoaded(name)) continue;
      const SkillInfo* skill = FindSkill(name);
      if (skill == nullptr) continue;
      try {
        Attach(*skill);
        loaded_.push_back(name);
      } catch (const std::exception&) {
        // not loadable now (another source has taken a tool name since): left out
      }
    }
  } catch (...) {
    // Restore is called by a hook and by /reset and must not throw
  }
}

std::string SkillTools::Catalog() const {
  std::string list;
  for (const SkillInfo& skill : catalog_.skills) list += (list.empty() ? "" : "\n") + std::string("- ") + skill.name + ": " + skill.description;
  return list;
}

std::vector<ToolSpec> SkillTools::Specs() const {
  json::Value names = json::Array();
  for (const SkillInfo& skill : catalog_.skills) json::Push(names, skill.name);
  const auto name_schema = [&](const char* what) {
    return json::Value{{"type", "object"},
                       {"properties", {{"name", {{"type", "string"}, {"description", what}, {"enum", names}}}}},
                       {"required", json::Array({"name"})},
                       {"additionalProperties", false}};
  };

  ToolSpec load;
  load.name = kLoad;
  load.description =
      "Loads a Skill: you get its instructions, and the tools it brings can be used from the next step on. Load a Skill when the "
      "task needs it; the Skills and what they are for are listed in the instructions.";
  load.parameters = name_schema("the Skill to load");
  load.effect = Effect::kSideEffect;

  ToolSpec unload;
  unload.name = kUnload;
  unload.description = "Unloads a Skill that was loaded: the tools it brought are taken away (unless another loaded Skill brings them too).";
  unload.parameters = name_schema("the Skill to unload");
  unload.effect = Effect::kSideEffect;
  return {load, unload};
}

boost::asio::awaitable<ToolResult> SkillTools::Call(const std::string& name, const json::Value& arguments) {
  const std::string skill_name = json::StringOr(arguments, "name", "");
  const SkillInfo* skill = FindSkill(skill_name);
  if (skill == nullptr) {
    std::vector<std::string> names;
    for (const SkillInfo& known : catalog_.skills) names.push_back(known.name);
    co_return Failure("there is no Skill called \"" + skill_name + "\"; the Skills are: " + Joined(names));
  }

  if (name == kLoad) {
    if (IsLoaded(skill_name)) co_return Failure("the Skill \"" + skill_name + "\" is already loaded");
    try {
      Attach(*skill);
    } catch (const std::exception& e) {
      co_return Failure("the Skill \"" + skill_name + "\" could not be loaded: " + e.what());
    }
    loaded_.push_back(skill_name);
    std::string text = "The Skill \"" + skill_name + "\" is loaded. Tools you can use now: " + Joined(skill->tools) + ".";
    if (!skill->body.empty()) text += "\n\n" + skill->body;
    co_return ToolResult{text, false};
  }

  if (name == kUnload) {
    if (!IsLoaded(skill_name)) co_return Failure("the Skill \"" + skill_name + "\" is not loaded");
    const std::vector<std::string> removed = Detach(*skill);
    loaded_.erase(std::find(loaded_.begin(), loaded_.end(), skill_name));
    std::string text = "The Skill \"" + skill_name + "\" is unloaded.";
    if (!removed.empty()) text += " Tools taken away: " + Joined(removed) + ".";
    co_return ToolResult{text, false};
  }

  co_return Failure("there is no tool called \"" + name + "\"");
}

boost::asio::awaitable<void> SkillTools::OnTurnStarted(const TurnStarted&) {
  before_turn_ = loaded_;
  co_return;
}

boost::asio::awaitable<void> SkillTools::OnTurnFinished(const TurnFinished& finished) {
  if (finished.outcome.reason != TurnOutcome::Reason::kDone) Restore(before_turn_);
  co_return;
}

}  // namespace agent
