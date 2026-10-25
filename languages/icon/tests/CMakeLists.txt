# The grammar itself: every example parses, and the tree holds what it
# declares.
add_executable(icon_grammar_test grammar_test.cpp)
target_link_libraries(icon_grammar_test PRIVATE iconCore)
target_compile_definitions(icon_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME icon_grammar_test COMMAND icon_grammar_test)

# ...and the command line reads one of the same samples.
add_test(NAME icon_cli_test COMMAND icon_cli
         ${PROJECT_SOURCE_DIR}/example/hello.icn)
