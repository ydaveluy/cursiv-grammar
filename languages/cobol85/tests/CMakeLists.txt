# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses clean.
add_executable(cobol85_grammar_test grammar_test.cpp)
target_link_libraries(cobol85_grammar_test PRIVATE cobol85Core)
target_compile_definitions(cobol85_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME cobol85_grammar_test COMMAND cobol85_grammar_test)

# ...and the command line reads one sample from it.
if(TARGET cobol85_cli)
  add_test(NAME cobol85_cli_test COMMAND cobol85_cli
           ${PROJECT_SOURCE_DIR}/example/example1.cbl)
endif()
