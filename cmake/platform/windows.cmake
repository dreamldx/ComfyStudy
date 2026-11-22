# Build settings for Windows with MSVC. The top-level CMakeLists.txt includes this file; it is the
# only file that knows about MSVC and vcpkg.
if(NOT MSVC)
  message(FATAL_ERROR "ComfyStudy is built with MSVC on Windows (run from an x64 Native Tools prompt).")
endif()

# Dynamic CRT: /MD in Release, /MDd in Debug.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# Compiler options for every target.
set(COMFYSTUDY_COMPILE_OPTIONS
  /W4 /WX /permissive- /utf-8 /Zc:__cplusplus /external:anglebrackets /external:W0)

# Third-party libraries come from vcpkg (see vcpkg.json); the preset points CMake at its toolchain file.
find_package(nlohmann_json CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)

# Called by the top-level CMakeLists.txt once the targets exist.
function(comfystudy_platform_targets)
  # Visual Studio only: make the program the startup project, so F5 runs it.
  set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT comfystudy)
endfunction()
