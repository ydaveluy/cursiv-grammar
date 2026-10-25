# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every corpus document parses, and select_features.rq
# is read back field by field.
add_executable(sparql_grammar_test grammar_test.cpp)
target_link_libraries(sparql_grammar_test PRIVATE sparqlCore)
target_compile_definitions(sparql_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME sparql_grammar_test COMMAND sparql_grammar_test)

# ...and the command line reads one document from the corpus.
if(TARGET sparql_cli)
  add_test(NAME sparql_cli_test COMMAND sparql_cli
           ${PROJECT_SOURCE_DIR}/example/example3.rq)
endif()
