# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document in example/ parses clean.
add_executable(graphstreamdgs_grammar_test grammar_test.cpp)
target_link_libraries(graphstreamdgs_grammar_test PRIVATE graphstreamdgsCore)
target_compile_definitions(graphstreamdgs_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME graphstreamdgs_grammar_test COMMAND graphstreamdgs_grammar_test)

# ...and the command line reads one sample.
if(TARGET graphstreamdgs_cli)
  add_test(NAME graphstreamdgs_cli_test COMMAND graphstreamdgs_cli
           ${PROJECT_SOURCE_DIR}/example/elements.dgs)
endif()
