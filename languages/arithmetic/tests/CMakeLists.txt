# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses clean into a File.
add_executable(arithmetic_grammar_test grammar_test.cpp)
target_link_libraries(arithmetic_grammar_test PRIVATE arithmeticCore)
target_compile_definitions(arithmetic_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME arithmetic_grammar_test COMMAND arithmetic_grammar_test)

# ...and the command line reads one example.
if(TARGET arithmetic_cli)
  add_test(NAME arithmetic_cli_test COMMAND arithmetic_cli
           ${PROJECT_SOURCE_DIR}/example/simple.arithmetic)
endif()
