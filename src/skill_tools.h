#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "agent_hook.h"
#include "native_tools.h"
#include "skill.h"
#include "tool_runtime.h"

namespace agent {

// The Skill mechanism (chapter 1.5): the two tools load_skill and unload_skill, and what they do to the
// tool runtime.
//
// The tools that Skills bring are written once, in `library` (a NativeTools that is not added to the
// runtime itself, so the model does not see them). Loading a Skill adds the tools it names to the runtime;
// unloading takes them out again. A tool that several loaded Skills name is added when the first of them is
// loaded and taken out when the last of them is unloaded (a count for each tool name).
//
// The model chooses from the list of Skills in the description of load_skill: the name and the task of each,
// not the tools (it learns those when the Skill is loaded).
//
// SkillTools is also a hook: a Turn that does not end in an answer is forgotten by the loop, and so are the
// Skills that were loaded during it; otherwise the model would not remember asking for the tools that are
// then in the list.
class SkillTools final : public ToolSource, public AgentHook {
 public:
  static constexpr const char* kLoad = "load_skill";
  static constexpr const char* kUnload = "unload_skill";

  // `runtime` is the one this source is added to; it has to be there for as long as this one is. Throws
  // std::invalid_argument when a Skill names a tool that `library` does not have.
  SkillTools(SkillCatalog catalog, std::shared_ptr<NativeTools> library, ToolRuntime& runtime);

  // The Skills that are loaded, in the order they were loaded.
  const std::vector<std::string>& Loaded() const { return loaded_; }

  // Loads and unloads until exactly these Skills are loaded (a Skill that is not in the catalog or cannot
  // be loaded is left out). Does not throw.
  void Restore(const std::vector<std::string>& loaded);

  std::vector<ToolSpec> Specs() const override;
  boost::asio::awaitable<ToolResult> Call(const std::string& name, const json::Value& arguments) override;
  void StartOver() override { Restore({}); }

  boost::asio::awaitable<void> OnTurnStarted(const TurnStarted&) override;
  boost::asio::awaitable<void> OnTurnFinished(const TurnFinished& finished) override;

 private:
  struct Count {
    int skills = 0;
    std::shared_ptr<ToolSource> source;
  };
  const SkillInfo* FindSkill(const std::string& name) const;
  bool IsLoaded(const std::string& name) const;
  void Attach(const SkillInfo& skill);                   // throws std::invalid_argument, leaving nothing behind
  std::vector<std::string> Detach(const SkillInfo& skill);  // the tools that were taken out of the runtime

  SkillCatalog catalog_;
  std::shared_ptr<NativeTools> library_;
  ToolRuntime& runtime_;
  std::vector<std::string> loaded_;
  std::vector<std::string> before_turn_;
  std::map<std::string, Count> counts_;
};

}  // namespace agent
