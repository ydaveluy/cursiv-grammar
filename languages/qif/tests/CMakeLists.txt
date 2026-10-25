# The grammar itself: every example parses clean, with nothing left over.
add_executable(qif_grammar_test grammar_test.cpp)
target_link_libraries(qif_grammar_test PRIVATE qifCore)
target_compile_definitions(qif_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME qif_grammar_test COMMAND qif_grammar_test)

# ...and the command line reads one of the same samples.
add_test(NAME qif_cli_test COMMAND qif_cli
         ${PROJECT_SOURCE_DIR}/example/example1.qif)
