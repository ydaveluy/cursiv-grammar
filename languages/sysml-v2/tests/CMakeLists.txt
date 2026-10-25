# The grammar itself: every example parses, and one document's fields hold
# what it declares.
add_executable(sysmlv2_grammar_test grammar_test.cpp)
target_link_libraries(sysmlv2_grammar_test PRIVATE sysmlv2Core)
target_compile_definitions(sysmlv2_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME sysmlv2_grammar_test COMMAND sysmlv2_grammar_test)

# ...and the command line reads one example.
add_test(NAME sysmlv2_cli_test COMMAND sysmlv2_cli
         ${PROJECT_SOURCE_DIR}/example/toaster-system.sysml)
