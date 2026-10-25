# The grammar itself: every example parses clean into a Document.
add_executable(fasta_grammar_test grammar_test.cpp)
target_link_libraries(fasta_grammar_test PRIVATE fastaCore cursiv::test cursiv::lsp)
target_compile_definitions(fasta_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME fasta_grammar_test COMMAND fasta_grammar_test)

# ...and the command line reads one example.
add_test(NAME fasta_cli_test COMMAND fasta_cli
         ${PROJECT_SOURCE_DIR}/example/NC_009925.fna.fasta)
