# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: the upstream corpus and this port's own coverage
# document both parse as expected.
add_executable(ocl_grammar_test grammar_test.cpp)
target_link_libraries(ocl_grammar_test PRIVATE oclCore cursiv::test)
target_compile_definitions(ocl_grammar_test PRIVATE
    CURSIV_EXAMPLE_SAMPLE="${PROJECT_SOURCE_DIR}/example/contextExamples.ocl"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME ocl_grammar_test COMMAND ocl_grammar_test)

# ...and the command line reads the upstream sample.
if(TARGET ocl_cli)
  add_test(NAME ocl_cli_test COMMAND ocl_cli
           ${PROJECT_SOURCE_DIR}/example/contextExamples.ocl)
endif()
