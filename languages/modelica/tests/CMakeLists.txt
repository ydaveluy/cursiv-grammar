# The grammar itself: every document under example/ and tests/ parses, and
# the tree holds what it declares.
add_executable(modelica_grammar_test grammar_test.cpp)
target_link_libraries(modelica_grammar_test PRIVATE modelicaCore)
target_compile_definitions(modelica_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME modelica_grammar_test COMMAND modelica_grammar_test)

# ...and the command line reads one of the same documents.
add_test(NAME modelica_cli_test COMMAND modelica_cli
         ${PROJECT_SOURCE_DIR}/example/example1.mo)
