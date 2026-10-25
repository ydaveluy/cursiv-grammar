# The grammar itself: the upstream corpus and this port's own coverage
# document both parse as expected.
add_executable(ocl_grammar_test grammar_test.cpp)
target_link_libraries(ocl_grammar_test PRIVATE oclCore)
target_compile_definitions(ocl_grammar_test PRIVATE
    CURSIV_EXAMPLE_SAMPLE="${PROJECT_SOURCE_DIR}/example/contextExamples.ocl"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME ocl_grammar_test COMMAND ocl_grammar_test)

# ...and the command line reads the upstream sample.
add_test(NAME ocl_cli_test COMMAND ocl_cli
         ${PROJECT_SOURCE_DIR}/example/contextExamples.ocl)
