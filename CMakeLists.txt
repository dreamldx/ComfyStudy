cmake_minimum_required(VERSION 3.25)

project(ComfyStudy VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Everything that depends on the operating system lives in cmake/platform/<system>.cmake: the
# compiler options, how the third-party libraries are found, and sources and targets that exist on
# that system only. This branch has windows.cmake only.
string(TOLOWER "${CMAKE_SYSTEM_NAME}" comfystudy_platform)
set(comfystudy_platform_file "${CMAKE_SOURCE_DIR}/cmake/platform/${comfystudy_platform}.cmake")
if(NOT EXISTS "${comfystudy_platform_file}")
  message(FATAL_ERROR "No build support for ${CMAKE_SYSTEM_NAME} on this branch "
                      "(missing ${comfystudy_platform_file})")
endif()
include("${comfystudy_platform_file}")

# Executables (and, later, the shared libraries they need) end up in build/<preset>/bin.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Compiler options shared by every target (the platform file chooses them).
add_library(comfystudy_options INTERFACE)
target_compile_options(comfystudy_options INTERFACE ${COMFYSTUDY_COMPILE_OPTIONS})

# Everything except main() lives in a static library, so tests can link it too.
add_library(comfystudy_core STATIC src/greeting.cpp)
target_include_directories(comfystudy_core PUBLIC src)
target_link_libraries(comfystudy_core PUBLIC comfystudy_options)

add_executable(comfystudy src/main.cpp)
target_link_libraries(comfystudy PRIVATE comfystudy_core)

# Targets that exist on one system only (defined by the platform file, now that the program exists).
comfystudy_platform_targets()

# Tests: one program per tests/test_*.cpp, run with CTest.
enable_testing()
add_subdirectory(tests)
