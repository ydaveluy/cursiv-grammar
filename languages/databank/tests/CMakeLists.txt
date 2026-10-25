# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every file of example/ parses clean into a Series.
add_executable(databank_grammar_test grammar_test.cpp)
target_link_libraries(databank_grammar_test PRIVATE databankCore)
target_compile_definitions(databank_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME databank_grammar_test COMMAND databank_grammar_test)

# ...and the command line reads one of them.
if(TARGET databank_cli)
  add_test(NAME databank_cli_test COMMAND databank_cli
           ${PROJECT_SOURCE_DIR}/example/example1.databank)
endif()
