# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses clean into a Document.
add_executable(vmf_grammar_test grammar_test.cpp)
target_link_libraries(vmf_grammar_test PRIVATE vmfCore)
target_compile_definitions(vmf_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME vmf_grammar_test COMMAND vmf_grammar_test)

# ...and the command line reads one example.
if(TARGET vmf_cli)
  add_test(NAME vmf_cli_test COMMAND vmf_cli
           ${PROJECT_SOURCE_DIR}/example/example1.vmf)
endif()
