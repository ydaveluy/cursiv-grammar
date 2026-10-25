# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example/ document parses clean.
add_executable(sapf_grammar_test grammar_test.cpp)
target_link_libraries(sapf_grammar_test PRIVATE sapfCore)
target_compile_definitions(sapf_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME sapf_grammar_test COMMAND sapf_grammar_test)

# ...and the command line reads one of them.
if(TARGET sapf_cli)
  add_test(NAME sapf_cli_test COMMAND sapf_cli
           ${PROJECT_SOURCE_DIR}/example/example28.sapf)
endif()
