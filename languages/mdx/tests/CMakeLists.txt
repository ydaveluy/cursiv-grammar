# The grammar itself: every corpus example parses clean, and one document's
# fields read back what it declares.
add_executable(mdx_grammar_test grammar_test.cpp)
target_link_libraries(mdx_grammar_test PRIVATE mdxCore)
target_compile_definitions(mdx_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
add_test(NAME mdx_grammar_test COMMAND mdx_grammar_test)

# ...and the command line reads one of the same examples.
add_test(NAME mdx_cli_test COMMAND mdx_cli
         ${PROJECT_SOURCE_DIR}/example/example1.mdx)
