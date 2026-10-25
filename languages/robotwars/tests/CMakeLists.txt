add_executable(robotwars_grammar_test grammar_test.cpp)
target_link_libraries(robotwars_grammar_test PRIVATE robotwarsCore)
target_compile_definitions(robotwars_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME robotwars_grammar_test COMMAND robotwars_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME robotwars_cli_test COMMAND robotwars_cli
         ${PROJECT_SOURCE_DIR}/example/scanner.robotwars)
