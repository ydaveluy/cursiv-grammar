# The grammar itself: the upstream corpus (example/) and coverage.ics (ours)
# parse clean, and the validator catches what the grammar alone accepts.
add_executable(icalendar_grammar_test grammar_test.cpp)
target_link_libraries(icalendar_grammar_test PRIVATE icalendarCore cursiv::test
                                                      cursiv::lsp)
target_compile_definitions(icalendar_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME icalendar_grammar_test COMMAND icalendar_grammar_test)

# ...and the command line reads one document from the same corpus.
add_test(NAME icalendar_cli_test COMMAND icalendar_cli
         ${PROJECT_SOURCE_DIR}/example/example1.ics)
