# The grammar itself: every sample under example/ parses clean.
add_executable(microc_grammar_test grammar_test.cpp)
target_link_libraries(microc_grammar_test PRIVATE microcCore)
target_compile_definitions(microc_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME microc_grammar_test COMMAND microc_grammar_test)

# ...and the command line reads one of them.
add_test(NAME microc_cli_test COMMAND microc_cli
         ${PROJECT_SOURCE_DIR}/example/example1.microc)
