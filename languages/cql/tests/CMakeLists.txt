# The grammar itself: every document under example/ parses clean.
add_executable(cql_grammar_test grammar_test.cpp)
target_link_libraries(cql_grammar_test PRIVATE cqlCore)
target_compile_definitions(cql_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME cql_grammar_test COMMAND cql_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME cql_cli_test COMMAND cql_cli
         ${PROJECT_SOURCE_DIR}/example/example1.cql)
