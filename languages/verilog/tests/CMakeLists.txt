# The grammar itself: every example parses, and the tree holds what it
# declares.
add_executable(verilog_grammar_test grammar_test.cpp)
target_link_libraries(verilog_grammar_test PRIVATE verilogCore)
target_compile_definitions(verilog_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME verilog_grammar_test COMMAND verilog_grammar_test)

# ...and the command line reads one of the same samples.
add_test(NAME verilog_cli_test COMMAND verilog_cli
         ${PROJECT_SOURCE_DIR}/example/wbuart.v)
