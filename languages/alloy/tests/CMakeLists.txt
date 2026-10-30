# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document in example/ parses, clean.
add_executable(alloy_grammar_test grammar_test.cpp)
target_link_libraries(alloy_grammar_test PRIVATE alloyCore cursiv::test)
target_compile_definitions(alloy_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME alloy_grammar_test COMMAND alloy_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET alloy_cli)
  add_test(NAME alloy_cli_test COMMAND alloy_cli
           ${PROJECT_SOURCE_DIR}/example/fs.als)
endif()
