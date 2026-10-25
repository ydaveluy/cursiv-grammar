# The grammar itself: every example parses clean, and hello_64.asm's values
# read back correctly.
add_executable(nasm_grammar_test grammar_test.cpp)
target_link_libraries(nasm_grammar_test PRIVATE nasmCore cursiv::test cursiv::lsp)
target_compile_definitions(nasm_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}/docs")
add_test(NAME nasm_grammar_test COMMAND nasm_grammar_test)

# ...and the command line reads the same sample.
add_test(NAME nasm_cli_test COMMAND nasm_cli
         ${PROJECT_SOURCE_DIR}/example/hello_64.asm)
