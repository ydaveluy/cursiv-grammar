# The grammar itself: every corpus document parses, plus the constructs the
# corpus never exercises, and one document is read back field by field.
add_executable(pf_grammar_test grammar_test.cpp)
target_link_libraries(pf_grammar_test PRIVATE pfCore)
target_compile_definitions(pf_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME pf_grammar_test COMMAND pf_grammar_test)

# ...and the command line reads one document from the same corpus.
add_test(NAME pf_cli_test COMMAND pf_cli
         ${PROJECT_SOURCE_DIR}/example/example11.pf)
