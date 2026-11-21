# Builds tests/<name>.cpp into a program that links the core library and registers it
# with CTest. A test passes when the program exits with code 0.
function(add_comfystudy_test name)
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} PRIVATE comfystudy_core)
  add_test(NAME ${name} COMMAND ${name})
endfunction()

add_comfystudy_test(test_greeting)
