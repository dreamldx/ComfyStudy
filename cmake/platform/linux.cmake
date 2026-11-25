# Build settings for Linux with GCC. The top-level CMakeLists.txt includes this file; on Linux it is the only
# file that knows about GCC and vcpkg. The linux-* presets (cmake/presets/linux.json) pick the compiler through
# cmake/toolchains/linux-gcc.cmake, so the program and the libraries vcpkg builds use the same GCC.
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  message(FATAL_ERROR "ComfyStudy is built with GCC on Linux (configure with one of the linux-* presets, "
                      "which select the pinned g++ through cmake/toolchains/linux-gcc.cmake).")
endif()

# Compiler options for every target.
set(COMFYSTUDY_COMPILE_OPTIONS -Wall -Wextra -Werror)

# Sanitizers: the linux-asan preset sets this to address,undefined. The options go to every target (tests too),
# for compiling and for linking.
set(COMFYSTUDY_SANITIZE "" CACHE STRING "Sanitizers for -fsanitize, comma separated (empty: none)")
if(COMFYSTUDY_SANITIZE)
  add_compile_options(-fsanitize=${COMFYSTUDY_SANITIZE} -fno-omit-frame-pointer -fno-sanitize-recover=all)
  add_link_options(-fsanitize=${COMFYSTUDY_SANITIZE})
endif()

# Third-party libraries come from vcpkg (see vcpkg.json); the preset points CMake at its toolchain file.
find_package(nlohmann_json CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)

# Sources and system libraries that exist on Linux only: none yet.
set(COMFYSTUDY_PLATFORM_SOURCES "")
set(COMFYSTUDY_PLATFORM_LIBRARIES "")

# Called by the top-level CMakeLists.txt once the targets exist. Linux has nothing here: the DLL management of
# the Windows build (deploy, check-dlls) has no counterpart, because vcpkg's libraries are static.
function(comfystudy_platform_targets)
endfunction()

# Called by tests/CMakeLists.txt once add_comfystudy_test exists: the tests that run on Linux only. None yet.
function(comfystudy_platform_tests)
endfunction()
