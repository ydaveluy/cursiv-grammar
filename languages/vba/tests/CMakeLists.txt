# The grammar itself: every document under example/ parses clean.
add_executable(vba_grammar_test grammar_test.cpp)
target_link_libraries(vba_grammar_test PRIVATE vbaCore)
target_compile_definitions(vba_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME vba_grammar_test COMMAND vba_grammar_test)

# ...and the command line reads one of them.
add_test(NAME vba_cli_test COMMAND vba_cli
         ${PROJECT_SOURCE_DIR}/example/SQLHelperFunctions.bas)
