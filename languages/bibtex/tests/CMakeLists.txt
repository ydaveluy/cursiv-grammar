# The lane of this language: one binary, the example directory is where its
# corpus is read from.
add_executable(bibtex_grammar_test grammar_test.cpp)
target_link_libraries(bibtex_grammar_test PRIVATE bibtexCore cursiv::test)
target_compile_definitions(bibtex_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME bibtex_grammar_test COMMAND bibtex_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME bibtex_cli_test COMMAND bibtex_cli
         ${PROJECT_SOURCE_DIR}/example/references.bib)
