#include <boost/asio.hpp>

#include <filesystem>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

#include "agent.h"
#include "chat.h"
#include "native_tools.h"
#include "skill.h"
#include "skill_tools.h"
#include "tool.h"
#include "tool_runtime.h"
#include "config.h"
#include "curl_transport.h"
#include "greeting.h"
#include "llm_client.h"
#include "net_probe.h"
#include "options.h"

#if defined(_WIN32)
#include "dll_info.h"
#endif

int main(int argc, char** argv) {
#if defined(_WIN32)
  // First thing: from now on DLLs are only loaded from this program's directory and System32.
  try {
    dllinfo::InitSearchPath();
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << '\n';
    return 1;
  }
#endif

  // Flush std::cout after every output, so its lines and spdlog's lines (which are written
  // immediately) stay in the order the program produced them even when stdout is redirected.
  std::cout << std::unitbuf;

  const std::vector<std::string> args = options::Arguments(argc, argv);
  const auto candidates =
      config::ConfigCandidates(config::ExeDirectory(), std::filesystem::current_path());
  const options::Result parsed = options::Parse(args, candidates, std::cout, std::cerr);
  if (parsed.exit_code) return *parsed.exit_code;  // --help, or a bad command line or config file

#if defined(_WIN32)
  if (parsed.options.dependency_check) {
    dllinfo::PrintDependencies(std::cout, dllinfo::StdoutSupportsColor());
    return 0;
  }
#endif

  spdlog::set_pattern("[%l] %v");
  if (parsed.options.verbose) spdlog::set_level(spdlog::level::debug);

  if (parsed.options.chat) {
    try {
      const config::Values& values = parsed.options.values;
      const std::vector<llm::Provider> providers = llm::Providers(values);
      if (providers.empty()) {
        std::cerr << "error: no provider is configured: set openai_model, openai_api_key and openai_base_url, "
                     "or anthropic_model, anthropic_api_key and anthropic_base_url\n";
        return 1;
      }
      const std::string start = llm::DefaultProviderName(values);
      llm::MakeProvider(values, start);  // throws and names the missing setting when the chosen one is incomplete
      const agent::TurnLimits limits = agent::LimitsFromSettings(values);
      // With --tools the model is offered the Skill mechanism (chapter 1.5). The toy tools are in a library that
      // the model does not see; a Skill brings some of them into the runtime when the model loads it. The runtime
      // checks, times and cuts every call.
      agent::ToolRuntime tools(agent::RuntimeOptionsFromSettings(values));
      std::shared_ptr<agent::SkillTools> skills;
      if (parsed.options.tools) {
        const auto library = std::make_shared<agent::NativeTools>();
        agent::AddToyTools(*library);
        const std::vector<std::string> names = library->Names();
        const auto found = values.find("skills_dir");
        const std::filesystem::path directory = found == values.end() || found->second.empty() ? "skills" : found->second;
        const agent::SkillCatalog catalog =
            agent::LoadSkills(directory, std::set<std::string>(names.begin(), names.end()),
                              {agent::SkillTools::kLoad, agent::SkillTools::kUnload});
        for (const std::string& problem : catalog.problems) spdlog::warn("Skill {}", problem);
        if (catalog.skills.empty()) throw std::runtime_error("there is no usable Skill in \"" + directory.string() + "\"");
        skills = std::make_shared<agent::SkillTools>(catalog, library, tools);
        tools.AddSource(skills);
      }
      agent::Hooks extra_hooks;
      if (skills) extra_hooks.push_back(skills);
      llm::UseUtf8Console();
      const auto transport = http::MakeCurlTransport();
      llm::RunChat(*transport, providers, start, llm::OptionsFromSettings(values), parsed.options.system,
                   parsed.options.tools ? &tools : nullptr, limits, std::cin, std::cout, std::cerr, extra_hooks);
      return 0;
    } catch (const std::exception& e) {
      std::cerr << "error: " << e.what() << '\n';
      return 1;
    }
  }

  if (!parsed.options.prompt.empty()) {
    try {
      // Ask is a coroutine: start it on an event loop and run the loop until it is done.
      boost::asio::io_context io;
      const auto transport = http::MakeCurlTransport();
      auto answer = boost::asio::co_spawn(
          io, llm::Ask(*transport, parsed.options.values, parsed.options.prompt, parsed.options.system),
          boost::asio::use_future);
      io.run();
      const llm::Reply reply = answer.get();  // throws what Ask threw
      std::cout << reply.Text() << '\n';
      spdlog::info("stopped because: {}; tokens in/out: {}/{}", reply.stop_reason,
                   reply.input_tokens.value_or(-1), reply.output_tokens.value_or(-1));
      return 0;
    } catch (const std::exception& e) {
      std::cerr << "error: " << e.what() << '\n';
      return 1;
    }
  }

  std::cout << Greeting() << '\n';
  std::cout << CurlVersion() << '\n';

  for (const auto& candidate : candidates) {
    spdlog::debug("config candidate {}: {}", candidate.string(),
                  std::filesystem::exists(candidate) ? "found" : "not found");
  }
  if (parsed.options.config_file) {
    spdlog::info("loaded {}", parsed.options.config_file->string());
  } else {
    spdlog::warn(
        "no config file found (looked next to the executable and in the current directory); "
        "using the environment and the defaults only");
  }

  config::Print(std::cout, parsed.options.values);
  return 0;
}
