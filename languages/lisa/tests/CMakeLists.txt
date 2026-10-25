# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every file of example/ parses clean.
add_executable(lisa_grammar_test grammar_test.cpp)
target_link_libraries(lisa_grammar_test PRIVATE lisaCore)
target_compile_definitions(lisa_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME lisa_grammar_test COMMAND lisa_grammar_test)

# ...and the command line reads one of the same samples.
if(TARGET lisa_cli)
  add_test(NAME lisa_cli_test COMMAND lisa_cli
           ${PROJECT_SOURCE_DIR}/example/example1.lisa)
endif()
