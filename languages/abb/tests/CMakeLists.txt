# The grammar itself: every document under example/ parses clean.
add_executable(abb_grammar_test grammar_test.cpp)
target_link_libraries(abb_grammar_test PRIVATE abbCore cursiv::test)
target_compile_definitions(abb_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME abb_grammar_test COMMAND abb_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME abb_cli_test COMMAND abb_cli
         ${PROJECT_SOURCE_DIR}/example/robdata.sys)
