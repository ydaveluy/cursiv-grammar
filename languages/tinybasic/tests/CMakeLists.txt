# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example and test document parses clean into a
# Program.
add_executable(tinybasic_grammar_test grammar_test.cpp)
target_link_libraries(tinybasic_grammar_test PRIVATE tinybasicCore)
target_compile_definitions(tinybasic_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME tinybasic_grammar_test COMMAND tinybasic_grammar_test)

# ...and the command line reads one of the same examples.
if(TARGET tinybasic_cli)
  add_test(NAME tinybasic_cli_test COMMAND tinybasic_cli
           ${PROJECT_SOURCE_DIR}/example/example1.tinybasic)
endif()
