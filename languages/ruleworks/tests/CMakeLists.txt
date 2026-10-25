# The grammar itself: every corpus example and the coverage document parse,
# and the tree holds what they declare. CURSIV_EXAMPLE_DIR is the upstream
# corpus; CURSIV_TEST_DIR is this directory, for documents of our own.
add_executable(ruleworks_grammar_test grammar_test.cpp)
target_link_libraries(ruleworks_grammar_test PRIVATE ruleworksCore cursiv::test
                                                      cursiv::lsp)
target_compile_definitions(ruleworks_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME ruleworks_grammar_test COMMAND ruleworks_grammar_test)

# ...and the command line reads one corpus example.
add_test(NAME ruleworks_cli_test COMMAND ruleworks_cli
         ${PROJECT_SOURCE_DIR}/example/count.rul)
