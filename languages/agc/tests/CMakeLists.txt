# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every corpus document parses clean, and one of them is
# read back field by field.
add_executable(agc_grammar_test grammar_test.cpp)
target_link_libraries(agc_grammar_test PRIVATE agcCore)
target_compile_definitions(agc_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME agc_grammar_test COMMAND agc_grammar_test)

# ...and the command line reads one document of the same corpus.
if(TARGET agc_cli)
  add_test(NAME agc_cli_test COMMAND agc_cli
           ${PROJECT_SOURCE_DIR}/example/DUMMY_501_INITIALIZATION.agc)
endif()
