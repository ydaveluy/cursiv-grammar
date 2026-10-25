# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses clean, and real field values read
# back out of the tree match the source.
add_executable(peoplecode_grammar_test grammar_test.cpp)
target_link_libraries(peoplecode_grammar_test PRIVATE peoplecodeCore)
target_compile_definitions(peoplecode_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME peoplecode_grammar_test COMMAND peoplecode_grammar_test)

# ...and the command line reads one sample from the same corpus.
if(TARGET peoplecode_cli)
  add_test(NAME peoplecode_cli_test COMMAND peoplecode_cli
           ${CMAKE_CURRENT_SOURCE_DIR}/coverage.pc)
endif()
