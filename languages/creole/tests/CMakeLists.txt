# The grammar itself: every sample under example/ parses clean.
add_executable(creole_grammar_test grammar_test.cpp)
target_link_libraries(creole_grammar_test PRIVATE creoleCore)
target_compile_definitions(creole_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME creole_grammar_test COMMAND creole_grammar_test)

# ...and the command line reads one of the same samples.
add_test(NAME creole_cli_test COMMAND creole_cli
         ${PROJECT_SOURCE_DIR}/example/complete.creole)
