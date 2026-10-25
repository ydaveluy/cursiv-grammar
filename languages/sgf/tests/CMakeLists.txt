# The grammar itself: every sample parses, and the tree holds a game tree.
add_executable(sgf_grammar_test grammar_test.cpp)
target_link_libraries(sgf_grammar_test PRIVATE sgfCore)
target_compile_definitions(sgf_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME sgf_grammar_test COMMAND sgf_grammar_test)

# ...and the command line reads one of the same samples.
add_test(NAME sgf_cli_test COMMAND sgf_cli
         ${PROJECT_SOURCE_DIR}/example/8718-KuroNeko-creedfoxx-chensicha.sgf)
