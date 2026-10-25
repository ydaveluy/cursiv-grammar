# The grammar itself: every document under example/ parses, and the tree
# holds what it declares.
add_executable(rfc822_grammar_test grammar_test.cpp)
target_link_libraries(rfc822_grammar_test PRIVATE rfc822Core cursiv::test)
target_compile_definitions(rfc822_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME rfc822_grammar_test COMMAND rfc822_grammar_test)

# ...and the command line reads one of them.
add_test(NAME rfc822_cli_test COMMAND rfc822_cli
         ${PROJECT_SOURCE_DIR}/example/example1.rfc822)
