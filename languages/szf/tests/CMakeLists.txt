# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses, and the tree holds a File.
add_executable(szf_grammar_test grammar_test.cpp)
target_link_libraries(szf_grammar_test PRIVATE szfCore)
target_compile_definitions(szf_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME szf_grammar_test COMMAND szf_grammar_test)

# ...and the command line reads one of the same examples.
if(TARGET szf_cli)
  add_test(NAME szf_cli_test COMMAND szf_cli
           ${PROJECT_SOURCE_DIR}/example/example1.szf)
endif()
