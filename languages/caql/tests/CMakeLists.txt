# The grammar itself: every document under example/ parses, and the tree
# holds what it declares.
add_executable(caql_grammar_test grammar_test.cpp)
target_link_libraries(caql_grammar_test PRIVATE caqlCore)
target_compile_definitions(caql_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME caql_grammar_test COMMAND caql_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME caql_cli_test COMMAND caql_cli
         ${PROJECT_SOURCE_DIR}/example/s1.caql)
