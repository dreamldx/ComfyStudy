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
add_comfystudy_test(test_llm_wire)
add_comfystudy_test(test_transport)
add_comfystudy_test(test_llm_client)
# These two start a small HTTP server on the loopback interface.
target_link_libraries(test_transport PRIVATE httplib::httplib)
target_link_libraries(test_llm_client PRIVATE httplib::httplib)
add_comfystudy_test(test_asio_basics)
target_link_libraries(test_asio_basics PRIVATE comfystudy_asio)
# Tests that exist on one system only are registered by the platform file.
comfystudy_platform_tests()
