# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses clean into a Document.
add_executable(recfile_grammar_test grammar_test.cpp)
target_link_libraries(recfile_grammar_test PRIVATE recfileCore)
target_compile_definitions(recfile_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME recfile_grammar_test COMMAND recfile_grammar_test)

# ...and the command line reads one example.
if(TARGET recfile_cli)
  add_test(NAME recfile_cli_test COMMAND recfile_cli
           ${PROJECT_SOURCE_DIR}/example/example1.rec)
endif()

add_executable(recfile_native_test native_test.cpp)
target_link_libraries(recfile_native_test PRIVATE recfileCore cursiv::test)
target_compile_definitions(recfile_native_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME recfile_native_test COMMAND recfile_native_test)
