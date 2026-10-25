# The grammar itself: every example parses clean into a Program.
add_executable(tinyc_grammar_test grammar_test.cpp)
target_link_libraries(tinyc_grammar_test PRIVATE tinycCore)
target_compile_definitions(tinyc_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME tinyc_grammar_test COMMAND tinyc_grammar_test)

# ...and the command line reads one example.
add_test(NAME tinyc_cli_test COMMAND tinyc_cli
         ${PROJECT_SOURCE_DIR}/example/example1.tinyc)
