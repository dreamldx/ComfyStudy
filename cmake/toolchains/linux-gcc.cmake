# One pinned GCC for everything on Linux. vcpkg chain-loads this file (triplets/x64-linux-gcc.cmake) to build the
# libraries, and the Linux CMake presets will use it for the project itself, so both are built by the same compiler.
#
# The major version must be the same as GCC_MAJOR in scripts/check-env.sh; tests/scripts/test-check-env.sh checks that.
# nvcc (chapter platform-1.5) must accept this major version as its host compiler.
set(COMFYSTUDY_GCC_MAJOR 16)

set(CMAKE_C_COMPILER gcc-${COMFYSTUDY_GCC_MAJOR})
set(CMAKE_CXX_COMPILER g++-${COMFYSTUDY_GCC_MAJOR})
