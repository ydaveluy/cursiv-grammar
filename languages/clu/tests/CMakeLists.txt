# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses clean into a Module.
add_executable(clu_grammar_test grammar_test.cpp)
target_link_libraries(clu_grammar_test PRIVATE cluCore)
target_compile_definitions(clu_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME clu_grammar_test COMMAND clu_grammar_test)

# ...and the command line reads one of the same examples.
if(TARGET clu_cli)
  add_test(NAME clu_cli_test COMMAND clu_cli
           ${PROJECT_SOURCE_DIR}/example/bottles-133.clu)
endif()
