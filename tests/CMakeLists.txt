find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

# Builds tests/<name>.cpp into a program that links the core library and GoogleTest, and
# registers every TEST in it with CTest (one CTest entry per TEST).
function(add_comfystudy_test name)
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} PRIVATE comfystudy_core GTest::gtest_main)
  gtest_discover_tests(${name})
endfunction()

add_comfystudy_test(test_greeting)
add_comfystudy_test(test_deps)
add_comfystudy_test(test_logging)
add_comfystudy_test(test_options)
add_comfystudy_test(test_config)
# Tests that exist on one system only are registered by the platform file.
comfystudy_platform_tests()
