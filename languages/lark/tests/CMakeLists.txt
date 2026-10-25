# The grammar itself: every document under example/ parses clean.
add_executable(lark_grammar_test grammar_test.cpp)
target_link_libraries(lark_grammar_test PRIVATE larkCore cursiv::test cursiv::lsp)
target_compile_definitions(lark_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME lark_grammar_test COMMAND lark_grammar_test)

# ...and the command line reads one of them.
add_test(NAME lark_cli_test COMMAND lark_cli
         ${PROJECT_SOURCE_DIR}/example/json.lark)
