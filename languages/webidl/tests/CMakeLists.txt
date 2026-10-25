# The grammar itself: every example parses, and the tree holds what it
# declares.
add_executable(webidl_grammar_test grammar_test.cpp)
target_link_libraries(webidl_grammar_test PRIVATE webidlCore cursiv::test cursiv::lsp)
target_compile_definitions(webidl_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME webidl_grammar_test COMMAND webidl_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME webidl_cli_test COMMAND webidl_cli
         ${PROJECT_SOURCE_DIR}/example/html_link_element.webidl)
