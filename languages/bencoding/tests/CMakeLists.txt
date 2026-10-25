# The grammar itself: every document under example/ parses clean.
add_executable(bencoding_grammar_test grammar_test.cpp)
target_link_libraries(bencoding_grammar_test PRIVATE bencodingCore cursiv::test cursiv::lsp)
target_compile_definitions(bencoding_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME bencoding_grammar_test COMMAND bencoding_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME bencoding_cli_test COMMAND bencoding_cli
         ${PROJECT_SOURCE_DIR}/example/list.torrent)

add_executable(bencoding_reference_table_test reference_table_test.cpp)
target_link_libraries(bencoding_reference_table_test PRIVATE bencodingCore)
add_test(NAME bencoding_reference_table_test COMMAND bencoding_reference_table_test)
