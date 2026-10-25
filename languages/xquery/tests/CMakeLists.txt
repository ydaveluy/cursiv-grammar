# The grammar itself: every corpus document parses, features.xq covers what
# the corpus doesn't, and module.xq is read back field by field.
add_executable(xquery_grammar_test grammar_test.cpp)
target_link_libraries(xquery_grammar_test PRIVATE xqueryCore)
target_compile_definitions(xquery_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_FEATURES_SAMPLE="${CMAKE_CURRENT_SOURCE_DIR}/features.xq")
add_test(NAME xquery_grammar_test COMMAND xquery_grammar_test)

# ...and the command line reads one document from the same corpus.
add_test(NAME xquery_cli_test COMMAND xquery_cli
         ${PROJECT_SOURCE_DIR}/example/module.xq)
