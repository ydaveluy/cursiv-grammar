# The grammar itself: every example parses, and a sample's tree holds what
# it declares.
add_executable(acme_grammar_test grammar_test.cpp)
target_link_libraries(acme_grammar_test PRIVATE acmeCore cursiv::test cursiv::lsp)
target_compile_definitions(acme_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME acme_grammar_test COMMAND acme_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME acme_cli_test COMMAND acme_cli
         ${PROJECT_SOURCE_DIR}/example/ClientAndServerFam.acme)
