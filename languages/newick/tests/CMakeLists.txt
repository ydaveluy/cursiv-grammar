# The grammar itself: every example parses clean, and the tree holds what it
# declares.
add_executable(newick_grammar_test grammar_test.cpp)
target_link_libraries(newick_grammar_test PRIVATE newickCore)
target_compile_definitions(newick_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME newick_grammar_test COMMAND newick_grammar_test)

# ...and the command line reads one of the same examples.
add_test(NAME newick_cli_test COMMAND newick_cli
         ${PROJECT_SOURCE_DIR}/example/example7.nwk)
