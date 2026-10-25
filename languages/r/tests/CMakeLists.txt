# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses, and the tree holds a Program.
add_executable(r_grammar_test grammar_test.cpp)
target_link_libraries(r_grammar_test PRIVATE rCore)
target_compile_definitions(r_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
add_test(NAME r_grammar_test COMMAND r_grammar_test)

# ...and the command line reads one of the same examples.
if(TARGET r_cli)
  add_test(NAME r_cli_test COMMAND r_cli
           ${PROJECT_SOURCE_DIR}/example/Complete.R)
endif()
