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

# Canaries: tests/sanitizer_canaries.cpp triggers one bug per run, and one test per bug passes only if the sanitizer
# reports it. On by default when a sanitizer is on. Turning it on without a sanitizer makes every canary fail,
# which is how you see that the check notices a missing sanitizer.
if(COMFYSTUDY_SANITIZE)
  set(comfystudy_canaries_default ON)
else()
  set(comfystudy_canaries_default OFF)
endif()
set(COMFYSTUDY_CANARIES ${comfystudy_canaries_default} CACHE BOOL "Build the sanitizer canaries and register their tests")

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

# One canary test: runs sanitizer_canaries with the name of the bug, and passes only if the output has the sentence
# the sanitizer prints for that bug. The exit code does not matter, so a plain crash does not pass.
function(comfystudy_add_canary name sentence)
  add_test(NAME canary.${name} COMMAND sanitizer_canaries ${name})
  set_tests_properties(canary.${name} PROPERTIES PASS_REGULAR_EXPRESSION "${sentence}" LABELS canary)
endfunction()

# Called by tests/CMakeLists.txt once add_comfystudy_test exists: the tests that run on Linux only, which are the
# canaries. They are registered for the sanitizers that are on (all of them when none is, so that they fail).
function(comfystudy_platform_tests)
  if(NOT COMFYSTUDY_CANARIES)
    return()
  endif()
  add_executable(sanitizer_canaries sanitizer_canaries.cpp)
  target_link_libraries(sanitizer_canaries PRIVATE comfystudy_options)

  # AddressSanitizer, and LeakSanitizer which comes with it.
  if(NOT COMFYSTUDY_SANITIZE OR COMFYSTUDY_SANITIZE MATCHES "address")
    comfystudy_add_canary(heap-overflow "AddressSanitizer: heap-buffer-overflow")
    comfystudy_add_canary(heap-use-after-free "AddressSanitizer: heap-use-after-free")
    comfystudy_add_canary(stack-overflow "AddressSanitizer: stack-buffer-overflow")
    comfystudy_add_canary(stack-use-after-return "AddressSanitizer: stack-use-after-return")
    comfystudy_add_canary(global-overflow "AddressSanitizer: global-buffer-overflow")
    comfystudy_add_canary(double-free "AddressSanitizer: attempting double-free")
    comfystudy_add_canary(leak "LeakSanitizer: detected memory leaks")
  endif()

  # UndefinedBehaviorSanitizer.
  if(NOT COMFYSTUDY_SANITIZE OR COMFYSTUDY_SANITIZE MATCHES "undefined")
    comfystudy_add_canary(signed-overflow "runtime error: signed integer overflow")
    comfystudy_add_canary(shift "runtime error: shift exponent")
    comfystudy_add_canary(div-zero "runtime error: division by zero")
    comfystudy_add_canary(null "runtime error: load of null pointer")
  endif()
endfunction()
