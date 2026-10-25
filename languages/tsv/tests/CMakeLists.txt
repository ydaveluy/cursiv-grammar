# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every file in example/ parses clean.
add_executable(tsv_grammar_test grammar_test.cpp)
target_link_libraries(tsv_grammar_test PRIVATE tsvCore cursiv::test)
target_compile_definitions(tsv_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME tsv_grammar_test COMMAND tsv_grammar_test)

# ...and the command line reads one of the same files.
if(TARGET tsv_cli)
  add_test(NAME tsv_cli_test COMMAND tsv_cli
           ${PROJECT_SOURCE_DIR}/example/example1.tsv)
endif()
