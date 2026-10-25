# The grammar itself: every file of example/ parses clean.
add_executable(ada_grammar_test grammar_test.cpp)
target_link_libraries(ada_grammar_test PRIVATE adaCore cursiv::test cursiv::lsp)
target_compile_definitions(ada_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME ada_grammar_test COMMAND ada_grammar_test)

# ...and the command line reads one of them.
add_test(NAME ada_cli_test COMMAND ada_cli
         ${PROJECT_SOURCE_DIR}/example/pkg1.ada)
