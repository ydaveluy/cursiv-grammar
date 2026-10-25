add_executable(aql_grammar_test grammar_test.cpp)
target_link_libraries(aql_grammar_test PRIVATE aqlCore)
target_compile_definitions(aql_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME aql_grammar_test COMMAND aql_grammar_test)

# ...and the command line reads one document from the same corpus.
add_test(NAME aql_cli_test COMMAND aql_cli
         ${PROJECT_SOURCE_DIR}/example/collect_1.aql)
