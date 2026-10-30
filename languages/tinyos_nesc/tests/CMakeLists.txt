# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

add_executable(tinyosnesc_grammar_test grammar_test.cpp)
target_link_libraries(tinyosnesc_grammar_test PRIVATE tinyosnescCore cursiv::test)
target_compile_definitions(tinyosnesc_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME tinyosnesc_grammar_test COMMAND tinyosnesc_grammar_test)

# ...and the command line reads one document from the corpus.
if(TARGET tinyosnesc_cli)
  add_test(NAME tinyosnesc_cli_test COMMAND tinyosnesc_cli
           ${PROJECT_SOURCE_DIR}/example/Blink/BlinkC.nc)
endif()
