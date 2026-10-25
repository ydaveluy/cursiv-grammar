# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses clean.
add_executable(codeql_grammar_test grammar_test.cpp)
target_link_libraries(codeql_grammar_test PRIVATE codeqlCore)
target_compile_definitions(codeql_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME codeql_grammar_test COMMAND codeql_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET codeql_cli)
  add_test(NAME codeql_cli_test COMMAND codeql_cli
           ${PROJECT_SOURCE_DIR}/example/types.ql)
endif()
