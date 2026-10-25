# The grammar itself: every corpus document parses, and hello.moo is read
# back field by field.
add_executable(moo_grammar_test grammar_test.cpp)
target_link_libraries(moo_grammar_test PRIVATE mooCore)
target_compile_definitions(moo_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_FEATURES_SAMPLE="${CMAKE_CURRENT_SOURCE_DIR}/features.moo")
add_test(NAME moo_grammar_test COMMAND moo_grammar_test)

# ...and the command line reads one document from the same corpus.
add_test(NAME moo_cli_test COMMAND moo_cli
         ${PROJECT_SOURCE_DIR}/example/hello.moo)
