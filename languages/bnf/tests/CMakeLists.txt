# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every sample parses, and each tree holds a Grammar root.
add_executable(bnf_grammar_test grammar_test.cpp)
target_link_libraries(bnf_grammar_test PRIVATE bnfCore)
target_compile_definitions(bnf_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME bnf_grammar_test COMMAND bnf_grammar_test)

# ...and the command line reads one of them.
if(TARGET bnf_cli)
  add_test(NAME bnf_cli_test COMMAND bnf_cli
           ${PROJECT_SOURCE_DIR}/example/wiki-bnf.bnf)
endif()
