# x64-linux (static libraries, the system's dynamic C and C++ runtime) with the compiler pinned by
# cmake/toolchains/linux-gcc.cmake. vcpkg ignores the CC and CXX environment variables, so a chain-loaded
# toolchain file is how every port gets built with the same GCC.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)

set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../cmake/toolchains/linux-gcc.cmake")
