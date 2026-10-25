# The grammar itself: every file of example/ parses, and the tree holds a
# proposition.
add_executable(ltl_grammar_test grammar_test.cpp)
target_link_libraries(ltl_grammar_test PRIVATE ltlCore)
target_compile_definitions(ltl_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME ltl_grammar_test COMMAND ltl_grammar_test)

# ...and the command line reads one of the same examples.
add_test(NAME ltl_cli_test COMMAND ltl_cli
         ${PROJECT_SOURCE_DIR}/example/example1.ltl)
