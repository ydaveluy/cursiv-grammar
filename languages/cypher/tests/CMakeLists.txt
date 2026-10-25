# The grammar itself: every document under example/ parses clean, plus the
# constructs this project's own tests/*.cypher cover.
add_executable(cypher_grammar_test grammar_test.cpp)
target_link_libraries(cypher_grammar_test PRIVATE cypherCore)
target_compile_definitions(cypher_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME cypher_grammar_test COMMAND cypher_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME cypher_cli_test COMMAND cypher_cli
         ${PROJECT_SOURCE_DIR}/example/match1.cypher)
