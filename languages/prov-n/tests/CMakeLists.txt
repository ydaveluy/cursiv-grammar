# The grammar itself: every corpus document parses, and two are read back
# field by field.
add_executable(provn_grammar_test grammar_test.cpp)
target_link_libraries(provn_grammar_test PRIVATE provnCore)
target_compile_definitions(provn_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_FEATURES_SAMPLE="${CMAKE_CURRENT_SOURCE_DIR}/features.provn")
add_test(NAME provn_grammar_test COMMAND provn_grammar_test)

# ...and the command line reads one document from the same corpus.
add_test(NAME provn_cli_test COMMAND provn_cli
         ${PROJECT_SOURCE_DIR}/example/example4.provn)
