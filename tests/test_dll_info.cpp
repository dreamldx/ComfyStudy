#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "dll_info.h"

namespace {

using dllinfo::Location;

const std::filesystem::path kApp = R"(D:\X\bin)";
const std::filesystem::path kWindows = R"(C:\Windows)";

// ---- Classify -------------------------------------------------------------------------------

TEST(Classify, TheProgramDirectoryIsApp) {
  EXPECT_EQ(dllinfo::Classify(R"(D:\X\bin\libcurl-d.dll)", kApp, kWindows), Location::App);
}

TEST(Classify, TheWindowsDirectoryAndItsSubdirectoriesAreWindows) {
  EXPECT_EQ(dllinfo::Classify(R"(C:\Windows\System32\kernel32.dll)", kApp, kWindows),
            Location::Windows);
  EXPECT_EQ(dllinfo::Classify(R"(C:\Windows\notepad.dll)", kApp, kWindows), Location::Windows);
}

TEST(Classify, AnythingElseIsOther) {
  EXPECT_EQ(dllinfo::Classify(R"(D:\CUDA\bin\cudart64_13.dll)", kApp, kWindows), Location::Other);
}

TEST(Classify, IgnoresCaseAndATrailingSeparator) {
  EXPECT_EQ(dllinfo::Classify(R"(d:\x\BIN\foo.dll)", R"(D:\X\bin\)", kWindows), Location::App);
}

TEST(Classify, ComparesWholeDirectoryNames) {
  // "bin2" starts with "bin" but is a different directory.
  EXPECT_EQ(dllinfo::Classify(R"(D:\X\bin2\foo.dll)", kApp, kWindows), Location::Other);
  EXPECT_EQ(dllinfo::Classify(R"(C:\Windows2\foo.dll)", kApp, kWindows), Location::Other);
}

TEST(Classify, SubdirectoriesOfTheProgramDirectoryAreApp) {
  EXPECT_EQ(dllinfo::Classify(R"(D:\X\bin\sub\foo.dll)", kApp, kWindows), Location::App);
}

// ---- FileVersion ----------------------------------------------------------------------------

TEST(FileVersion, ReadsTheVersionResourceOfASystemDll) {
  const std::filesystem::path kernel32 =
      dllinfo::WindowsDirectory() / "System32" / "kernel32.dll";
  EXPECT_TRUE(std::regex_match(dllinfo::FileVersion(kernel32), std::regex(R"(\d+\.\d+\.\d+\.\d+)")));
}

TEST(FileVersion, IsADashWhenThereIsNoVersion) {
  EXPECT_EQ(dllinfo::FileVersion(R"(D:\no\such\file.dll)"), "-");
}

// ---- manifest -------------------------------------------------------------------------------

TEST(Manifest, ParsesTabSeparatedLines) {
  std::istringstream in(
      "# comment\n"
      "libcurl-d.dll\tAAAA\tcurl\t8.23.0\n"
      "\n"
      "Hand-Made.dll\tBBBB\t-\t-\r\n"
      "broken line without tabs\n");
  const dllinfo::Manifest manifest = dllinfo::ParseManifest(in);
  ASSERT_EQ(manifest.size(), 2u);
  const auto& curl = manifest.at("libcurl-d.dll");
  EXPECT_EQ(curl.sha256, "AAAA");
  EXPECT_EQ(curl.package, "curl");
  EXPECT_EQ(curl.package_version, "8.23.0");
  const auto& hand = manifest.at("hand-made.dll");  // keys are lower case
  EXPECT_EQ(hand.package, "-");
  EXPECT_EQ(hand.package_version, "-");
}

TEST(Manifest, AMissingFileIsAnEmptyManifest) {
  EXPECT_TRUE(dllinfo::LoadManifest(R"(D:\no\such\dlls.manifest)").empty());
}

// ---- LoadedModules --------------------------------------------------------------------------

TEST(LoadedModules, ListsKernel32AndTheTestProgram) {
  bool kernel32 = false;
  bool program = false;
  for (const auto& module : dllinfo::LoadedModules()) {
    if (module.name == "KERNEL32.DLL" || module.name == "kernel32.dll") kernel32 = true;
    if (module.path == dllinfo::ProgramPath()) program = true;
    EXPECT_FALSE(module.name.empty());
  }
  EXPECT_TRUE(kernel32);
  EXPECT_TRUE(program);
}

// ---- Print ----------------------------------------------------------------------------------

class PrintTest : public ::testing::Test {
 protected:
  dllinfo::Context Context(bool color) {
    dllinfo::Context context;
    context.app_dir = kApp;
    context.windows_dir = kWindows;
    context.color = color;
    context.manifest["libcurl-d.dll"] = {"AAAA", "curl", "8.23.0"};
    context.manifest["handmade.dll"] = {"BBBB", "-", "-"};
    return context;
  }

  std::vector<dllinfo::Module> Modules() {
    return {
        {"kernel32.dll", R"(C:\Windows\System32\kernel32.dll)"},
        {"foo.dll", R"(D:\Other\foo.dll)"},
        {"handmade.dll", R"(D:\X\bin\handmade.dll)"},
        {"libcurl-d.dll", R"(D:\X\bin\libcurl-d.dll)"},
    };
  }

  static std::string Line(const std::string& text, const std::string& containing) {
    std::istringstream in(text);
    for (std::string line; std::getline(in, line);) {
      if (line.find(containing) != std::string::npos) return line;
    }
    return {};
  }

  std::string Render(bool color) {
    std::ostringstream out;
    dllinfo::Print(out, Modules(), Context(color));
    return out.str();
  }
};

TEST_F(PrintTest, ShowsTheLocationAndThePackage) {
  const std::string text = Render(false);
  const std::string curl = Line(text, "libcurl-d.dll");
  EXPECT_NE(curl.find("[bin ]"), std::string::npos) << curl;
  EXPECT_NE(curl.find("curl 8.23.0"), std::string::npos) << curl;
  EXPECT_NE(curl.find(R"(D:\X\bin\libcurl-d.dll)"), std::string::npos) << curl;
  EXPECT_NE(Line(text, "kernel32.dll").find("[sys ]"), std::string::npos);
  EXPECT_NE(Line(text, "foo.dll").find("[else]"), std::string::npos);
}

TEST_F(PrintTest, OnlyVcpkgDllsGetAStar) {
  const std::string text = Render(false);
  EXPECT_NE(Line(text, "libcurl-d.dll").find("libcurl-d.dll *"), std::string::npos);
  EXPECT_EQ(Line(text, "handmade.dll").find('*'), std::string::npos);  // in the manifest, no package
  EXPECT_EQ(Line(text, "kernel32.dll").find('*'), std::string::npos);  // not in the manifest
  EXPECT_EQ(Line(text, "foo.dll").find('*'), std::string::npos);
}

TEST_F(PrintTest, ADllThatIsNotInTheManifestHasADashAsPackage) {
  const std::string text = Render(false);
  const std::string line = Line(text, "kernel32.dll");
  EXPECT_NE(line.find(" - "), std::string::npos) << line;
}

TEST_F(PrintTest, TheLegendExplainsTheStarOnlyWhenThereIsOne) {
  EXPECT_NE(Render(false).find("* = installed by vcpkg"), std::string::npos);

  dllinfo::Context context = Context(false);
  context.manifest.clear();
  std::ostringstream out;
  dllinfo::Print(out, Modules(), context);
  EXPECT_EQ(out.str().find("installed by vcpkg"), std::string::npos);
}

TEST_F(PrintTest, ColorsAreOnlyWrittenWhenAsked) {
  EXPECT_EQ(Render(false).find('\x1b'), std::string::npos);

  const std::string text = Render(true);
  EXPECT_EQ(Line(text, "libcurl-d.dll").rfind("\x1b[32m", 0), 0u);   // program directory: green
  EXPECT_EQ(Line(text, "kernel32.dll").rfind("\x1b[90m", 0), 0u);    // Windows directory: gray
  EXPECT_EQ(Line(text, "foo.dll").rfind("\x1b[33m", 0), 0u);         // elsewhere: yellow
  EXPECT_NE(Line(text, "libcurl-d.dll").find("\x1b[0m"), std::string::npos);
}

TEST_F(PrintTest, ListsTheProgramDirectoryFirstAndTheWindowsDirectoryLast) {
  const std::string text = Render(false);
  const auto curl = text.find("libcurl-d.dll");
  const auto foo = text.find("foo.dll");
  const auto kernel32 = text.find("kernel32.dll");
  EXPECT_LT(curl, foo);
  EXPECT_LT(foo, kernel32);
}

}  // namespace
