add_executable(informix_grammar_test grammar_test.cpp)
target_link_libraries(informix_grammar_test PRIVATE informixCore)
target_compile_definitions(informix_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME informix_grammar_test COMMAND informix_grammar_test)

# ...and the command line reads one sample.
add_test(NAME informix_cli_test COMMAND informix_cli
         ${PROJECT_SOURCE_DIR}/example/example2.4gl)
