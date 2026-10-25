# The grammar itself: every example parses clean into an Equation.
add_executable(calculator_grammar_test grammar_test.cpp)
target_link_libraries(calculator_grammar_test PRIVATE calculatorCore)
target_compile_definitions(calculator_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME calculator_grammar_test COMMAND calculator_grammar_test)

# ...and the command line reads one example.
add_test(NAME calculator_cli_test COMMAND calculator_cli
         ${PROJECT_SOURCE_DIR}/example/area.calc)
