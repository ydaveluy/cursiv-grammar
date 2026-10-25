# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses clean, and one tree holds what it
# declares.
add_executable(z_grammar_test grammar_test.cpp)
target_link_libraries(z_grammar_test PRIVATE zCore)
target_compile_definitions(z_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME z_grammar_test COMMAND z_grammar_test)

# ...and the command line reads one of the same examples.
if(TARGET z_cli)
  add_test(NAME z_cli_test COMMAND z_cli
           ${PROJECT_SOURCE_DIR}/example/ss.zed)
endif()
