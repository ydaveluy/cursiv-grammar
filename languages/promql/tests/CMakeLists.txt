# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses, and the tree
# holds what it declares.
add_executable(promql_grammar_test grammar_test.cpp)
target_link_libraries(promql_grammar_test PRIVATE promqlCore)
target_compile_definitions(promql_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME promql_grammar_test COMMAND promql_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET promql_cli)
  add_test(NAME promql_cli_test COMMAND promql_cli
           ${PROJECT_SOURCE_DIR}/example/vector_match_operation.promql)
endif()
