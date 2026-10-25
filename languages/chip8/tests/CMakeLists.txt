# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses, and every opcode is pinned.
add_executable(chip8_grammar_test grammar_test.cpp)
target_link_libraries(chip8_grammar_test PRIVATE chip8Core)
target_compile_definitions(chip8_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME chip8_grammar_test COMMAND chip8_grammar_test)

# ...and the command line reads one of the same examples.
if(TARGET chip8_cli)
  add_test(NAME chip8_cli_test COMMAND chip8_cli
           ${PROJECT_SOURCE_DIR}/example/example1.chip8)
endif()
