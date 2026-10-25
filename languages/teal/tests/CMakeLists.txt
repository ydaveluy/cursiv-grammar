# The grammar itself: every example parses, and a few field values are read
# back out of the AST.
add_executable(teal_grammar_test grammar_test.cpp)
target_link_libraries(teal_grammar_test PRIVATE tealCore)
target_compile_definitions(teal_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME teal_grammar_test COMMAND teal_grammar_test)

# ...and the command line reads one of the same samples.
add_test(NAME teal_cli_test COMMAND teal_cli
         ${PROJECT_SOURCE_DIR}/example/functions.tl)
