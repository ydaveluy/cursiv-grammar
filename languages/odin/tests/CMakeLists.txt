# The grammar itself: every corpus document parses, and one is read back
# field by field.
add_executable(odin_grammar_test grammar_test.cpp)
target_link_libraries(odin_grammar_test PRIVATE odinCore)
target_compile_definitions(odin_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME odin_grammar_test COMMAND odin_grammar_test)

# ...and the command line reads one document from the same corpus.
add_test(NAME odin_cli_test COMMAND odin_cli
         ${PROJECT_SOURCE_DIR}/example/test.odin)
