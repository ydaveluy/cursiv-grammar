# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

add_executable(bicep_grammar_test grammar_test.cpp)
target_link_libraries(bicep_grammar_test PRIVATE bicepCore)
target_compile_definitions(bicep_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME bicep_grammar_test COMMAND bicep_grammar_test)

# ...and the command line reads one corpus sample.
if(TARGET bicep_cli)
  add_test(NAME bicep_cli_test COMMAND bicep_cli
           ${PROJECT_SOURCE_DIR}/example/program.bicep)
endif()
