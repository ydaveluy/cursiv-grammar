# The grammar itself: every document under tests/ (the upstream corpus is
# empty) parses, and real field values read back correctly.
add_executable(graphql_grammar_test grammar_test.cpp)
target_link_libraries(graphql_grammar_test PRIVATE graphqlCore)
target_compile_definitions(graphql_grammar_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME graphql_grammar_test COMMAND graphql_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME graphql_cli_test COMMAND graphql_cli
         ${CMAKE_CURRENT_SOURCE_DIR}/query.graphql)
