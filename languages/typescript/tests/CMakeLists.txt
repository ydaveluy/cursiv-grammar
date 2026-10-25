# The grammar itself: every example parses, and the tree holds a Program.
add_executable(typescript_grammar_test grammar_test.cpp)
target_link_libraries(typescript_grammar_test PRIVATE typescriptCore cursiv::test)
target_compile_definitions(typescript_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME typescript_grammar_test COMMAND typescript_grammar_test)

# ...and the command line reads one of the same examples.
add_test(NAME typescript_cli_test COMMAND typescript_cli
         ${PROJECT_SOURCE_DIR}/example/Class.ts)
