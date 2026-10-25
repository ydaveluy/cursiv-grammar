add_executable(cto_grammar_test grammar_test.cpp)
target_link_libraries(cto_grammar_test PRIVATE ctoCore)
target_compile_definitions(cto_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME cto_grammar_test COMMAND cto_grammar_test)

# ...and the command line reads one document of the corpus.
add_test(NAME cto_cli_test COMMAND cto_cli
         ${PROJECT_SOURCE_DIR}/example/car.cto)
