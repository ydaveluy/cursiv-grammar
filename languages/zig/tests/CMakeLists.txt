# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses as expected, and one document's
# fields read back.
add_executable(zig_grammar_test grammar_test.cpp)
target_link_libraries(zig_grammar_test PRIVATE zigCore)
target_compile_definitions(zig_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CURSIV_INVALID_SAMPLE="${CMAKE_CURRENT_SOURCE_DIR}/invalid.zig")
add_test(NAME zig_grammar_test COMMAND zig_grammar_test)

# ...and the command line reads one clean sample.
if(TARGET zig_cli)
  add_test(NAME zig_cli_test COMMAND zig_cli
           ${PROJECT_SOURCE_DIR}/example/f2.zig)
endif()
