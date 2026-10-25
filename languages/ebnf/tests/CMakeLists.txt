# The grammar itself: every example parses, and each tree holds what it declares.
add_executable(ebnf_grammar_test grammar_test.cpp)
target_link_libraries(ebnf_grammar_test PRIVATE ebnfCore)
target_compile_definitions(ebnf_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME ebnf_grammar_test COMMAND ebnf_grammar_test)

# ...and the command line reads one of them.
add_test(NAME ebnf_cli_test COMMAND ebnf_cli
         ${PROJECT_SOURCE_DIR}/example/postal.ebnf)
