# The grammar itself: every example under example/ parses clean.
add_executable(kirikiritjs_grammar_test grammar_test.cpp)
target_link_libraries(kirikiritjs_grammar_test PRIVATE kirikiritjsCore)
target_compile_definitions(kirikiritjs_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME kirikiritjs_grammar_test COMMAND kirikiritjs_grammar_test)

# ...and the command line reads one of the same examples.
add_test(NAME kirikiritjs_cli_test COMMAND kirikiritjs_cli
         ${PROJECT_SOURCE_DIR}/example/test_scripts/test_class.tjs)
