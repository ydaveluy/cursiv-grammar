# The grammar itself: every example parses, and the sample holds what it declares.
add_executable(oncrpc_grammar_test grammar_test.cpp)
target_link_libraries(oncrpc_grammar_test PRIVATE oncrpcCore cursiv::test)
target_compile_definitions(oncrpc_grammar_test PRIVATE
    CURSIV_SAMPLE="${PROJECT_SOURCE_DIR}/example/CalculatorService.x"
    CURSIV_EXAMPLE_TYPES="${CMAKE_CURRENT_SOURCE_DIR}/types.x")
add_test(NAME oncrpc_grammar_test COMMAND oncrpc_grammar_test)

# ...and the command line reads the same sample.
add_test(NAME oncrpc_cli_test COMMAND oncrpc_cli
         ${PROJECT_SOURCE_DIR}/example/CalculatorService.x)
