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
find_package(fmt CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)

# Sources and system libraries that exist on Windows only.
set(COMFYSTUDY_PLATFORM_SOURCES src/dll_info.cpp)
set(COMFYSTUDY_PLATFORM_LIBRARIES
  psapi     # list the loaded modules
  version)  # read the version resources of DLLs

# Called by the top-level CMakeLists.txt once the targets exist.
function(comfystudy_platform_targets)
  # Visual Studio only: make the program the startup project, so F5 runs it.
  set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT comfystudy)

  # DLL management. vcpkg copies the DLLs a program needs next to it (build/<preset>/bin).
  #   deploy      writes bin/dlls.manifest: every DLL there with its SHA-256 and vcpkg package.
  #               `comfystudy --dependency-check` reads it to show which DLLs come from vcpkg.
  #   check-dlls  fails if a program needs a DLL that is neither a system DLL nor in the manifest.
  find_program(COMFYSTUDY_POWERSHELL NAMES pwsh powershell REQUIRED)
  add_custom_target(deploy
    COMMAND ${COMFYSTUDY_POWERSHELL} -NoProfile -ExecutionPolicy Bypass
            -File ${CMAKE_SOURCE_DIR}/tools/write-manifest.ps1
            -BinDir $<TARGET_FILE_DIR:comfystudy>
            -VcpkgInfoDir ${VCPKG_INSTALLED_DIR}/vcpkg/info
    COMMENT "Writing bin/dlls.manifest"
    VERBATIM)
  add_dependencies(deploy comfystudy)
  add_custom_target(check-dlls
    COMMAND ${COMFYSTUDY_POWERSHELL} -NoProfile -ExecutionPolicy Bypass
            -File ${CMAKE_SOURCE_DIR}/tools/check-dlls.ps1
            -BinDir $<TARGET_FILE_DIR:comfystudy>
    DEPENDS deploy
    COMMENT "Checking the DLLs the programs need"
    VERBATIM)
endfunction()

# Called by tests/CMakeLists.txt once add_comfystudy_test exists: the tests that run on Windows only.
function(comfystudy_platform_tests)
  add_comfystudy_test(test_dll_info)

  # DLL checks (they need the `deploy` and `check-dlls` targets above).
  # check_dlls builds both targets, so dlls.manifest exists afterwards; dependency_check uses it.
  add_test(NAME check_dlls
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config $<CONFIG> --target check-dlls)
  set_tests_properties(check_dlls PROPERTIES FIXTURES_SETUP dll_manifest)

  # --dependency-check only reports, but with the manifest it must mark libcurl as a vcpkg DLL.
  add_test(NAME dependency_check COMMAND comfystudy --dependency-check)
  set_tests_properties(dependency_check PROPERTIES
    FIXTURES_REQUIRED dll_manifest
    PASS_REGULAR_EXPRESSION "libcurl[a-z-]*\\.dll \\*")
endfunction()
