# The grammar itself: every corpus document parses clean, plus a document of
# ours exercising constructs the corpus never uses.
add_executable(langium_grammar_test grammar_test.cpp)
target_link_libraries(langium_grammar_test PRIVATE langiumCore)
target_compile_definitions(langium_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME langium_grammar_test COMMAND langium_grammar_test)

# ...and the command line reads one document from the same corpus.
add_test(NAME langium_cli_test COMMAND langium_cli
         ${PROJECT_SOURCE_DIR}/example/sql.langium)
