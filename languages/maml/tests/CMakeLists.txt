# The grammar itself: every example parses, and the tree holds what it
# declares.
add_executable(maml_grammar_test grammar_test.cpp)
target_link_libraries(maml_grammar_test PRIVATE mamlCore)
target_compile_definitions(maml_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME maml_grammar_test COMMAND maml_grammar_test)

# ...and the command line reads one of the same examples.
add_test(NAME maml_cli_test COMMAND maml_cli
         ${PROJECT_SOURCE_DIR}/example/basic.maml)
