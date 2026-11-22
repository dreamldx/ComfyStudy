#pragma once

#include <filesystem>
#include <iosfwd>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace dllinfo {

// Makes the program look for DLLs only in its own directory and in System32 (not in PATH or the
// current directory) from now on. Call it first in main(); throws std::runtime_error on failure.
void InitSearchPath();

// A DLL (or the program itself) loaded into this process.
struct Module {
  std::string name;             // file name, e.g. "libcurl-d.dll"
  std::filesystem::path path;   // full path
};

// Every module of the running process, in load order.
std::vector<Module> LoadedModules();

// Full path of the running executable, and the Windows directory (usually C:\Windows).
std::filesystem::path ProgramPath();
std::filesystem::path WindowsDirectory();

// Where a module was loaded from.
enum class Location {
  App,      // the program's own directory (or below it)
  Windows,  // the Windows directory (or below it, e.g. System32)
  Other,    // anywhere else
};

// Directories are compared name by name, ignoring case and a trailing separator, so ...\bin2 is not
// inside ...\bin.
Location Classify(const std::filesystem::path& module, const std::filesystem::path& app_dir,
                  const std::filesystem::path& windows_dir);

// "10.0.26100.1" from the file's version resource, or "-" if it has none (or does not exist).
std::string FileVersion(const std::filesystem::path& dll);

// One line of dlls.manifest, written by the `deploy` target (tools/write-manifest.ps1):
//   name <TAB> sha256 <TAB> package <TAB> package_version
// `package` is the vcpkg package that installed the DLL, or "-" if it is not from vcpkg.
struct ManifestEntry {
  std::string sha256;
  std::string package;
  std::string package_version;
};
using Manifest = std::map<std::string, ManifestEntry>;  // key: DLL name in lower case

// Lines that are empty, start with '#', or do not have four tab-separated fields are skipped.
Manifest ParseManifest(std::istream& in);

// ParseManifest of a file; a missing or unreadable file gives an empty manifest.
Manifest LoadManifest(const std::filesystem::path& file);

struct Context {
  std::filesystem::path app_dir;
  std::filesystem::path windows_dir;
  Manifest manifest;
  bool color = false;  // write ANSI color codes
};

// One line per module: location tag, name (followed by '*' if vcpkg installed it), file version,
// package, full path; then a legend. It only reports; it never decides whether anything is wrong.
void Print(std::ostream& out, const std::vector<Module>& modules, const Context& context);

// Print() for the running process, with dlls.manifest read from the program's directory.
void PrintDependencies(std::ostream& out, bool color);

// True if colors should be used on stdout: it is a console that understands ANSI codes (they are
// switched on here) and the NO_COLOR environment variable is not set.
bool StdoutSupportsColor();

}  // namespace dllinfo
