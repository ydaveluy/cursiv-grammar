# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every file of example/ parses clean, and one of them is
# read back field by field.
add_executable(turtle_grammar_test grammar_test.cpp)
target_link_libraries(turtle_grammar_test PRIVATE turtleCore cursiv::test)
target_compile_definitions(turtle_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME turtle_grammar_test COMMAND turtle_grammar_test)

# ...and the command line reads one of them.
if(TARGET turtle_cli)
  add_test(NAME turtle_cli_test COMMAND turtle_cli
           ${PROJECT_SOURCE_DIR}/example/Vehicle.ttl)
endif()
