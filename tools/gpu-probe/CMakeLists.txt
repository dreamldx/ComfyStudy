# gpu_probe: can a program on this machine use the GPU through the NVIDIA driver?
#
# Part of the main project (a ctest test named gpu_probe), and also buildable on its own:
#   cmake -S tools/gpu-probe -B <dir> -G Ninja
# which is what scripts/check-env does, because it must work before the project's dependencies
# exist. Only the driver is needed, not the CUDA toolkit.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  cmake_minimum_required(VERSION 3.25)
  project(gpu_probe LANGUAGES CXX)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  enable_testing()
endif()

# The driver's own library: libcuda.so (the driver package provides the development link) or, on
# Windows, cuda.lib from the CUDA toolkit's import libraries. A machine without it is not an
# error here: the test is registered as disabled, so the rest of the project still builds.
find_library(COMFYSTUDY_CUDA_DRIVER_LIB NAMES cuda HINTS "$ENV{CUDA_PATH}/lib/x64")

if(NOT COMFYSTUDY_CUDA_DRIVER_LIB)
  message(STATUS "GPU: no CUDA driver library found (libcuda.so, or cuda.lib in CUDA_PATH), "
                 "the gpu_probe test is disabled")
  add_test(NAME gpu_probe COMMAND ${CMAKE_COMMAND} -E false)
  set_tests_properties(gpu_probe PROPERTIES DISABLED TRUE LABELS gpu)
  return()
endif()
message(STATUS "GPU: CUDA driver library ${COMFYSTUDY_CUDA_DRIVER_LIB}")

add_executable(gpu_probe probe.cpp)
target_link_libraries(gpu_probe PRIVATE "${COMFYSTUDY_CUDA_DRIVER_LIB}")
if(TARGET comfystudy_options)
  target_link_libraries(gpu_probe PRIVATE comfystudy_options)
elseif(MSVC)
  target_compile_options(gpu_probe PRIVATE /W4 /WX /permissive-)
else()
  target_compile_options(gpu_probe PRIVATE -Wall -Wextra -Werror)
endif()

add_test(NAME gpu_probe COMMAND gpu_probe)
set_tests_properties(gpu_probe PROPERTIES LABELS gpu SKIP_RETURN_CODE 77)
