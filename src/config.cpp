#include "config.h"

#include <stdlib.h>

#include <cctype>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace config {
namespace {

bool EndsWith(std::string_view s, std::string_view suffix) {
  return s.size() >= suffix.size() && s.substr(s.size() - suffix.size()) == suffix;
}

bool IsSecretName(std::string_view name) { return EndsWith(name, "_key") || EndsWith(name, "_token"); }

}  // namespace

std::string EnvName(std::string_view name) {
  std::string env(name);
  for (char& c : env) {
    c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  }
  return env;
}

const std::vector<Key>& KnownKeys() {
  static const std::vector<Key> keys = [] {
    const std::pair<const char*, const char*> settings[] = {
        {"discord_token", ""},
        {"llm_provider", "openai"},
        {"llm_reasoning", "off"},
        {"agent_max_steps", "10"},
        {"agent_turn_timeout_ms", "300000"},
        {"tool_timeout_ms", "30000"},
        {"tool_result_max_bytes", "16384"},
        {"skills_dir", "skills"},
        {"openai_api", "chat"},
        {"openai_model", ""},
        {"openai_api_key", ""},
        {"openai_base_url", "https://api.openai.com/v1"},
        {"anthropic_model", ""},
        {"anthropic_api_key", ""},
        {"anthropic_base_url", "https://api.anthropic.com"},
        {"model_dir", "models"},
    };
    std::vector<Key> out;
    for (const auto& [name, default_value] : settings) {
      out.push_back({name, EnvName(name), default_value, IsSecretName(name)});
    }
    return out;
  }();
  return keys;
}

std::optional<std::string> Get(std::string_view env_name) {
  std::string value;
#if defined(_WIN32)
  char* buffer = nullptr;
  std::size_t length = 0;
  if (_dupenv_s(&buffer, &length, std::string(env_name).c_str()) != 0 || buffer == nullptr) {
    return std::nullopt;
  }
  value = buffer;
  free(buffer);
#else
#error "config::Get is not implemented for this platform on this branch"
#endif
  if (value.empty()) return std::nullopt;
  return value;
}

std::string MaskSecret(std::string_view value) {
  if (value.empty()) return "(unset)";
  if (value.size() < 8) return "****";
  return std::string(value.substr(0, 3)) + "***";
}

std::string Describe(const Values& values, const Key& key) {
  std::string line = key.name + '=';
  const auto found = values.find(key.name);
  if (found == values.end() || found->second.empty()) return line + "(unset)";
  return line + (key.secret ? MaskSecret(found->second) : found->second);
}

void Print(std::ostream& out, const Values& values) {
  for (const Key& key : KnownKeys()) out << Describe(values, key) << '\n';
}

std::filesystem::path ExeDirectory() {
#if defined(_WIN32)
  std::wstring buffer(MAX_PATH, L'\0');
  for (;;) {
    const DWORD copied = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (copied == 0) return {};
    if (copied < buffer.size()) {
      buffer.resize(copied);
      break;
    }
    buffer.resize(buffer.size() * 2);  // the path did not fit: try a bigger buffer
  }
  return std::filesystem::path(buffer).parent_path();
#else
#error "config::ExeDirectory is not implemented for this platform on this branch"
#endif
}

std::vector<std::filesystem::path> ConfigCandidates(const std::filesystem::path& exe_dir,
                                                    const std::filesystem::path& cwd) {
  return {exe_dir / "config.toml", exe_dir / "config.ini", cwd / "config.toml", cwd / "config.ini"};
}

}  // namespace config
