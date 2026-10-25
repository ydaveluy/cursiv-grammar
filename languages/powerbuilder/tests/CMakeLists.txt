# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document in example/ parses, clean.
add_executable(powerbuilder_grammar_test grammar_test.cpp)
target_link_libraries(powerbuilder_grammar_test PRIVATE powerbuilderCore)
target_compile_definitions(powerbuilder_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME powerbuilder_grammar_test COMMAND powerbuilder_grammar_test)

# ...and the command line reads one of them.
if(TARGET powerbuilder_cli)
  add_test(NAME powerbuilder_cli_test COMMAND powerbuilder_cli
           ${PROJECT_SOURCE_DIR}/example/example4.sra)
endif()
