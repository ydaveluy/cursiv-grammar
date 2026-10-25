# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every corpus document parses clean, and a few are read
# back field by field to catch a converter that silently mismaps its value.
add_executable(wavefront_grammar_test grammar_test.cpp)
target_link_libraries(wavefront_grammar_test PRIVATE wavefrontCore)
target_compile_definitions(wavefront_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME wavefront_grammar_test COMMAND wavefront_grammar_test)

# ...and the command line reads one document from the same corpus.
if(TARGET wavefront_cli)
  add_test(NAME wavefront_cli_test COMMAND wavefront_cli
           ${PROJECT_SOURCE_DIR}/example/b1_20_cube.obj)
endif()
