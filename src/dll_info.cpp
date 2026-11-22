#include "dll_info.h"

#include <algorithm>
#include <cstdlib>
#include <cwctype>
#include <fstream>
#include <ostream>
#include <sstream>
#include <stdexcept>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>

#include <fmt/color.h>

namespace dllinfo {
namespace {

std::wstring Lower(std::wstring text) {
  std::transform(text.begin(), text.end(), text.begin(),
                 [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
  return text;
}

std::string LowerAscii(std::string text) {
  std::transform(text.begin(), text.end(), text.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return text;
}

// The names of the directories in `path`, without the root and without empty names (which a
// trailing separator would produce).
std::vector<std::wstring> Parts(const std::filesystem::path& path) {
  std::vector<std::wstring> parts;
  parts.push_back(Lower(path.root_name().wstring()));
  for (const auto& part : path.relative_path()) {
    if (!part.empty()) parts.push_back(Lower(part.wstring()));
  }
  return parts;
}

// True if `path` is `directory` or somewhere below it.
bool IsInside(const std::filesystem::path& path, const std::filesystem::path& directory) {
  const auto dir = Parts(directory);
  const auto sub = Parts(path);
  return sub.size() >= dir.size() && std::equal(dir.begin(), dir.end(), sub.begin());
}

std::filesystem::path FromWide(const std::wstring& text) {
  // GetModuleFileName may return "\\?\C:\...": drop the prefix.
  constexpr std::wstring_view kPrefix = LR"(\\?\)";
  if (text.compare(0, kPrefix.size(), kPrefix) == 0) return text.substr(kPrefix.size());
  return text;
}

const char* Tag(Location location) {
  switch (location) {
    case Location::App: return "[bin ]";
    case Location::Windows: return "[sys ]";
    default: return "[else]";
  }
}

// The color of a row: green for the program directory, gray for the Windows directory, yellow
// for everywhere else. fmt writes the ANSI codes (and the reset after the text) for us.
fmt::text_style Style(Location location) {
  switch (location) {
    case Location::App: return fmt::fg(fmt::terminal_color::green);
    case Location::Windows: return fmt::fg(fmt::terminal_color::bright_black);  // gray
    default: return fmt::fg(fmt::terminal_color::yellow);
  }
}

struct Row {
  Location location;
  std::string name;  // with a trailing " *" for vcpkg DLLs
  std::string version;
  std::string package;
  std::string path;
};

}  // namespace

void InitSearchPath() {
  if (!::SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
                                  LOAD_LIBRARY_SEARCH_SYSTEM32)) {
    throw std::runtime_error("SetDefaultDllDirectories failed, error " +
                             std::to_string(::GetLastError()));
  }
}

std::filesystem::path ProgramPath() {
  std::wstring buffer(MAX_PATH, L'\0');
  for (;;) {
    const DWORD length = ::GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0) return {};
    if (length < buffer.size()) {
      buffer.resize(length);
      return FromWide(buffer);
    }
    buffer.resize(buffer.size() * 2);  // it did not fit
  }
}

std::filesystem::path WindowsDirectory() {
  std::wstring buffer(MAX_PATH, L'\0');
  const UINT length = ::GetWindowsDirectoryW(buffer.data(), static_cast<UINT>(buffer.size()));
  buffer.resize(length);
  return buffer;
}

std::vector<Module> LoadedModules() {
  std::vector<HMODULE> handles(256);
  DWORD needed = 0;
  for (;;) {
    const DWORD bytes = static_cast<DWORD>(handles.size() * sizeof(HMODULE));
    if (!::EnumProcessModules(::GetCurrentProcess(), handles.data(), bytes, &needed)) return {};
    if (needed <= bytes) break;
    handles.resize(needed / sizeof(HMODULE));
  }
  handles.resize(needed / sizeof(HMODULE));

  std::vector<Module> modules;
  for (HMODULE handle : handles) {
    std::wstring buffer(32768, L'\0');
    const DWORD length = ::GetModuleFileNameW(handle, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0) continue;
    buffer.resize(length);
    Module module;
    module.path = FromWide(buffer);
    module.name = module.path.filename().string();
    modules.push_back(std::move(module));
  }
  return modules;
}

Location Classify(const std::filesystem::path& module, const std::filesystem::path& app_dir,
                  const std::filesystem::path& windows_dir) {
  const std::filesystem::path directory = module.parent_path();
  if (IsInside(directory, app_dir)) return Location::App;
  if (IsInside(directory, windows_dir)) return Location::Windows;
  return Location::Other;
}

std::string FileVersion(const std::filesystem::path& dll) {
  const std::wstring file = dll.wstring();
  DWORD ignored = 0;
  const DWORD size = ::GetFileVersionInfoSizeW(file.c_str(), &ignored);
  if (size == 0) return "-";
  std::vector<char> data(size);
  if (!::GetFileVersionInfoW(file.c_str(), 0, size, data.data())) return "-";
  VS_FIXEDFILEINFO* info = nullptr;
  UINT length = 0;
  if (!::VerQueryValueW(data.data(), L"\\", reinterpret_cast<void**>(&info), &length) || info == nullptr ||
      length < sizeof(VS_FIXEDFILEINFO) || info->dwSignature != 0xFEEF04BD) {
    return "-";
  }
  std::ostringstream out;
  out << HIWORD(info->dwFileVersionMS) << '.' << LOWORD(info->dwFileVersionMS) << '.'
      << HIWORD(info->dwFileVersionLS) << '.' << LOWORD(info->dwFileVersionLS);
  return out.str();
}

Manifest ParseManifest(std::istream& in) {
  Manifest manifest;
  for (std::string line; std::getline(in, line);) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line.empty() || line[0] == '#') continue;
    std::vector<std::string> fields;
    std::istringstream stream(line);
    for (std::string field; std::getline(stream, field, '\t');) fields.push_back(field);
    if (fields.size() != 4 || fields[0].empty()) continue;
    manifest[LowerAscii(fields[0])] = {fields[1], fields[2], fields[3]};
  }
  return manifest;
}

Manifest LoadManifest(const std::filesystem::path& file) {
  std::ifstream in(file);
  if (!in) return {};
  return ParseManifest(in);
}

void Print(std::ostream& out, const std::vector<Module>& modules, const Context& context) {
  std::vector<Row> rows;
  bool any_vcpkg = false;
  for (const Module& module : modules) {
    Row row;
    row.location = Classify(module.path, context.app_dir, context.windows_dir);
    row.name = module.name;
    row.version = FileVersion(module.path);
    row.package = "-";
    const auto found = context.manifest.find(LowerAscii(module.name));
    if (found != context.manifest.end() && found->second.package != "-") {
      row.name += " *";
      row.package = found->second.package;
      if (found->second.package_version != "-") row.package += " " + found->second.package_version;
      any_vcpkg = true;
    }
    row.path = module.path.string();
    rows.push_back(std::move(row));
  }

  // The program's directory first, the Windows directory last; by name inside each group.
  const auto rank = [](Location location) {
    return location == Location::App ? 0 : (location == Location::Other ? 1 : 2);
  };
  std::stable_sort(rows.begin(), rows.end(), [&](const Row& a, const Row& b) {
    if (rank(a.location) != rank(b.location)) return rank(a.location) < rank(b.location);
    return LowerAscii(a.name) < LowerAscii(b.name);
  });

  size_t name_width = 0, version_width = 0, package_width = 0;
  for (const Row& row : rows) {
    name_width = std::max(name_width, row.name.size());
    version_width = std::max(version_width, row.version.size());
    package_width = std::max(package_width, row.package.size());
  }

  // With color off, an empty style makes fmt write the text as it is.
  const auto paint = [&](Location location, const std::string& text) {
    return fmt::format(context.color ? Style(location) : fmt::text_style(), "{}", text);
  };
  for (const Row& row : rows) {
    const std::string line = fmt::format("{} {:<{}}  {:<{}}  {:<{}}  {}", Tag(row.location),
                                         row.name, name_width, row.version, version_width,
                                         row.package, package_width, row.path);
    out << paint(row.location, line) << '\n';
  }

  out << "\nLocation: " << paint(Location::App, "[bin ] program directory") << ", "
      << paint(Location::Windows, "[sys ] Windows directory") << ", "
      << paint(Location::Other, "[else] elsewhere") << '\n';
  if (any_vcpkg) out << "* = installed by vcpkg\n";
}

void PrintDependencies(std::ostream& out, bool color) {
  Context context;
  const std::filesystem::path program = ProgramPath();
  context.app_dir = program.parent_path();
  context.windows_dir = WindowsDirectory();
  context.manifest = LoadManifest(context.app_dir / "dlls.manifest");
  context.color = color;
  Print(out, LoadedModules(), context);
}

bool StdoutSupportsColor() {
  char* no_color = nullptr;
  size_t length = 0;
  const bool disabled = _dupenv_s(&no_color, &length, "NO_COLOR") == 0 && no_color != nullptr && *no_color != '\0';
  std::free(no_color);
  if (disabled) return false;

  const HANDLE handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode = 0;
  if (handle == INVALID_HANDLE_VALUE || !::GetConsoleMode(handle, &mode)) return false;  // not a console
  return ::SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
}

}  // namespace dllinfo
