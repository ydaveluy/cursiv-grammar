# The grammar itself: every example parses clean into a SegmentHeader.
add_executable(tcpheader_grammar_test grammar_test.cpp)
target_link_libraries(tcpheader_grammar_test PRIVATE tcpheaderCore cursiv::test)
target_compile_definitions(tcpheader_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME tcpheader_grammar_test COMMAND tcpheader_grammar_test)

# ...and the command line reads one example.
add_test(NAME tcpheader_cli_test COMMAND tcpheader_cli
         ${PROJECT_SOURCE_DIR}/example/example1.tcpheader)
