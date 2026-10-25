# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

add_executable(tnsnames_grammar_test grammar_test.cpp)
target_link_libraries(tnsnames_grammar_test PRIVATE tnsnamesCore)
target_compile_definitions(tnsnames_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME tnsnames_grammar_test COMMAND tnsnames_grammar_test)

# ...and the command line reads the same corpus file.
if(TARGET tnsnames_cli)
  add_test(NAME tnsnames_cli_test COMMAND tnsnames_cli
           ${PROJECT_SOURCE_DIR}/example/tnsnames.test.ora)
endif()
