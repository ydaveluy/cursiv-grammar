# The grammar itself: every example parses clean, and the tree holds what it
# declares.
add_executable(guitartab_grammar_test grammar_test.cpp)
target_link_libraries(guitartab_grammar_test PRIVATE guitartabCore)
target_compile_definitions(guitartab_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME guitartab_grammar_test COMMAND guitartab_grammar_test)

# ...and the command line reads one of the same examples.
add_test(NAME guitartab_cli_test COMMAND guitartab_cli
         ${PROJECT_SOURCE_DIR}/example/example1.guitartab)
