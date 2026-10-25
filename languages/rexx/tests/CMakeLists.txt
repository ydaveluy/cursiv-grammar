# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses, and the tree holds what it
# declares.
add_executable(rexx_grammar_test grammar_test.cpp)
target_link_libraries(rexx_grammar_test PRIVATE rexxCore)
target_compile_definitions(rexx_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME rexx_grammar_test COMMAND rexx_grammar_test)

# ...and the command line reads one of the same examples.
if(TARGET rexx_cli)
  add_test(NAME rexx_cli_test COMMAND rexx_cli
           ${PROJECT_SOURCE_DIR}/example/example1.rex)
endif()
