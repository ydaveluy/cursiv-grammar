# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses clean into a Program.
add_executable(flowmatic_grammar_test grammar_test.cpp)
target_link_libraries(flowmatic_grammar_test PRIVATE flowmaticCore cursiv::test)
target_compile_definitions(flowmatic_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME flowmatic_grammar_test COMMAND flowmatic_grammar_test)

# ...and the command line reads one of the same examples.
if(TARGET flowmatic_cli)
  add_test(NAME flowmatic_cli_test COMMAND flowmatic_cli
           ${PROJECT_SOURCE_DIR}/example/example1.flowmatic)
endif()
