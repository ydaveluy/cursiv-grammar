# The grammar itself: every sample under example/ parses clean.
add_executable(vhdl_grammar_test grammar_test.cpp)
target_link_libraries(vhdl_grammar_test PRIVATE vhdlCore)
target_compile_definitions(vhdl_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME vhdl_grammar_test COMMAND vhdl_grammar_test)

# ...and the command line reads one of them.
add_test(NAME vhdl_cli_test COMMAND vhdl_cli
         ${PROJECT_SOURCE_DIR}/example/add_pkg.vhd)
