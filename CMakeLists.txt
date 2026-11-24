cmake_minimum_required(VERSION 3.25)

project(ComfyStudy VERSION 1.1.0 LANGUAGES CXX)

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

# Boost.Asio: the event loop and coroutines. _WIN32_WINNT selects the Windows 10 API; the old
# interfaces that Asio has since replaced are switched off.
add_library(comfystudy_asio INTERFACE)
target_link_libraries(comfystudy_asio INTERFACE Boost::asio comfystudy_options)
target_include_directories(comfystudy_asio INTERFACE src)  # for src/event_loop.h
target_compile_definitions(comfystudy_asio INTERFACE _WIN32_WINNT=0x0A00 BOOST_ASIO_NO_DEPRECATED)

# Everything except main() lives in a static library, so tests can link it too.
# COMFYSTUDY_PLATFORM_SOURCES and COMFYSTUDY_PLATFORM_LIBRARIES come from the platform file.
add_library(comfystudy_core STATIC
  src/greeting.cpp src/net_probe.cpp src/options.cpp src/config.cpp
  src/http_client.cpp src/curl_transport.cpp src/json.cpp src/async_http.cpp src/llm_wire.cpp src/llm_client.cpp
  ${COMFYSTUDY_PLATFORM_SOURCES})
target_include_directories(comfystudy_core PUBLIC src)
target_link_libraries(comfystudy_core PUBLIC comfystudy_options comfystudy_asio
  glaze::glaze CURL::libcurl spdlog::spdlog fmt::fmt CLI11::CLI11
  ${COMFYSTUDY_PLATFORM_LIBRARIES})

add_executable(comfystudy src/main.cpp)
target_link_libraries(comfystudy PRIVATE comfystudy_core)

# A small program that shows the event loop at work (see docs: llm-1.1).
add_executable(asio_demo demo/asio_demo.cpp)
target_link_libraries(asio_demo PRIVATE comfystudy_asio)

# Measures the ways to send many requests with libcurl (side quest chapter llm-1.2). Not part of the program.
add_executable(curl_bench bench/curl_bench.cpp)
target_include_directories(curl_bench PRIVATE tests)  # for the small test server
target_link_libraries(curl_bench PRIVATE comfystudy_core httplib::httplib)

# Targets that exist on one system only (defined by the platform file, now that the program exists).
comfystudy_platform_targets()

# Tests: one program per tests/test_*.cpp, run with CTest.
enable_testing()
add_subdirectory(tests)

# Can this machine's GPU be used through the NVIDIA driver? A test (label gpu) that is skipped
# or disabled when there is no driver library or no device. See tools/gpu-probe/probe.cpp.
add_subdirectory(tools/gpu-probe)
