# The grammar itself: every document in example/ parses, clean.
add_executable(abnf_grammar_test grammar_test.cpp)
target_link_libraries(abnf_grammar_test PRIVATE abnfCore cursiv::test cursiv::lsp)
target_compile_definitions(abnf_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME abnf_grammar_test COMMAND abnf_grammar_test)

# ...and the command line reads one of them.
add_test(NAME abnf_cli_test COMMAND abnf_cli
         ${PROJECT_SOURCE_DIR}/example/abnf.abnf)
