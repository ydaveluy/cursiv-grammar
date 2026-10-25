# The grammar itself: every example under example/ parses, and each tree
# holds a Program.
add_executable(trac_grammar_test grammar_test.cpp)
target_link_libraries(trac_grammar_test PRIVATE tracCore)
target_compile_definitions(trac_grammar_test PRIVATE
    CURSIV_EXAMPLES_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME trac_grammar_test COMMAND trac_grammar_test)

# ...and the command line reads one of them.
add_test(NAME trac_cli_test COMMAND trac_cli
         ${PROJECT_SOURCE_DIR}/example/example1.trac)
