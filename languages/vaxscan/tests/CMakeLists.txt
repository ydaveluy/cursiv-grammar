# The grammar itself: every corpus document parses, and hanoi.scn is read
# back field by field.
add_executable(vaxscan_grammar_test grammar_test.cpp)
target_link_libraries(vaxscan_grammar_test PRIVATE vaxscanCore)
target_compile_definitions(vaxscan_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_FEATURES_SAMPLE="${CMAKE_CURRENT_SOURCE_DIR}/features.scn")
add_test(NAME vaxscan_grammar_test COMMAND vaxscan_grammar_test)

# ...and the command line reads one document from the same corpus.
add_test(NAME vaxscan_cli_test COMMAND vaxscan_cli
         ${PROJECT_SOURCE_DIR}/example/hanoi.scn)
