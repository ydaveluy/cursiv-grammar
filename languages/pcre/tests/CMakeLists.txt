# The grammar itself: every document under example/ parses clean, and one
# pattern is read back field by field.
add_executable(pcre_grammar_test grammar_test.cpp)
target_link_libraries(pcre_grammar_test PRIVATE pcreCore)
target_compile_definitions(pcre_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME pcre_grammar_test COMMAND pcre_grammar_test)

# ...and the command line reads one document from the same corpus.
add_test(NAME pcre_cli_test COMMAND pcre_cli
         ${PROJECT_SOURCE_DIR}/example/various.pcre)
