# The grammar itself: every document under example/ parses clean, and
# connectives.ctl's tree matches the language's operator precedence.
add_executable(ctl_grammar_test grammar_test.cpp)
target_link_libraries(ctl_grammar_test PRIVATE ctlCore cursiv::test)
target_compile_definitions(ctl_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME ctl_grammar_test COMMAND ctl_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME ctl_cli_test COMMAND ctl_cli
         ${PROJECT_SOURCE_DIR}/example/example1.ctl)
