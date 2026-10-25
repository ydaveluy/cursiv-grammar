# The grammar itself: every example parses, and the tree holds a Document.
add_executable(hypertalk_grammar_test grammar_test.cpp)
target_link_libraries(hypertalk_grammar_test PRIVATE hypertalkCore)
target_compile_definitions(hypertalk_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME hypertalk_grammar_test COMMAND hypertalk_grammar_test)

# ...and the command line reads one of the same examples.
add_test(NAME hypertalk_cli_test COMMAND hypertalk_cli
         ${PROJECT_SOURCE_DIR}/example/SingleLine.hypertalk)

# Constructs the six-file upstream corpus does not reach: chunk expressions,
# containers, `repeat` forms, multi-line `if`/`else`, `send`, `do`, property
# access. Read through CURSIV_TEST_DIR, never counted as example/ corpus.
add_executable(hypertalk_coverage_test coverage_test.cpp)
target_link_libraries(hypertalk_coverage_test PRIVATE hypertalkCore)
target_compile_definitions(hypertalk_coverage_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}/coverage")
add_test(NAME hypertalk_coverage_test COMMAND hypertalk_coverage_test)
