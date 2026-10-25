# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses clean into a Document.
add_executable(pbm_grammar_test grammar_test.cpp)
target_link_libraries(pbm_grammar_test PRIVATE pbmCore cursiv::test)
target_compile_definitions(pbm_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME pbm_grammar_test COMMAND pbm_grammar_test)

# ...and the command line reads one example.
if(TARGET pbm_cli)
  add_test(NAME pbm_cli_test COMMAND pbm_cli
           ${PROJECT_SOURCE_DIR}/example/example1.pbm)
endif()
