# The grammar itself: every file under example/ parses clean.
add_executable(trapc_grammar_test grammar_test.cpp)
target_link_libraries(trapc_grammar_test PRIVATE trapcCore)
target_compile_definitions(trapc_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME trapc_grammar_test COMMAND trapc_grammar_test)

# ...and the command line reads one of the same samples.
add_test(NAME trapc_cli_test COMMAND trapc_cli
         ${PROJECT_SOURCE_DIR}/example/trap_test.tc)
