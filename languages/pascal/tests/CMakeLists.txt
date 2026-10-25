# The grammar itself: every sample parses, and each tree holds a Program.
add_executable(pascal_grammar_test grammar_test.cpp)
target_link_libraries(pascal_grammar_test PRIVATE pascalCore cursiv::test)
target_compile_definitions(pascal_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME pascal_grammar_test COMMAND pascal_grammar_test)

# ...and the command line reads one of the same samples.
add_test(NAME pascal_cli_test COMMAND pascal_cli
         ${PROJECT_SOURCE_DIR}/example/helloworld.pas)
