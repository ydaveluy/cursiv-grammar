# The grammar itself: every document under example/ parses clean.
add_executable(thrift_grammar_test grammar_test.cpp)
target_link_libraries(thrift_grammar_test PRIVATE thriftCore)
target_compile_definitions(thrift_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME thrift_grammar_test COMMAND thrift_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME thrift_cli_test COMMAND thrift_cli
         ${PROJECT_SOURCE_DIR}/example/literal.thrift)
