# The grammar itself: every sample parses, and the tree holds what it declares.
add_executable(pl0_grammar_test grammar_test.cpp)
target_link_libraries(pl0_grammar_test PRIVATE pl0Core)
target_compile_definitions(pl0_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME pl0_grammar_test COMMAND pl0_grammar_test)

# ...and the command line reads one sample of the corpus.
add_test(NAME pl0_cli_test COMMAND pl0_cli
         ${PROJECT_SOURCE_DIR}/example/example1.pl0)
