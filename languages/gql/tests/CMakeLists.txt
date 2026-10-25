# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses clean.
add_executable(gql_grammar_test grammar_test.cpp)
target_link_libraries(gql_grammar_test PRIVATE gqlCore)
target_compile_definitions(gql_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME gql_grammar_test COMMAND gql_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET gql_cli)
  add_test(NAME gql_cli_test COMMAND gql_cli
           ${PROJECT_SOURCE_DIR}/example/01_simple_match.gql)
endif()
