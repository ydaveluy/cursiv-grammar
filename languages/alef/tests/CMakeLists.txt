# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: the upstream sample and this port's own document
# both parse, and the tree holds what each declares.
add_executable(alef_grammar_test grammar_test.cpp)
target_link_libraries(alef_grammar_test PRIVATE alefCore)
target_compile_definitions(alef_grammar_test PRIVATE
    CURSIV_EXAMPLE_SAMPLE="${PROJECT_SOURCE_DIR}/example/example1.alef"
    CURSIV_TEST_SAMPLE="${CMAKE_CURRENT_SOURCE_DIR}/features.alef")
add_test(NAME alef_grammar_test COMMAND alef_grammar_test)

# ...and the command line reads the upstream sample.
if(TARGET alef_cli)
  add_test(NAME alef_cli_test COMMAND alef_cli
           ${PROJECT_SOURCE_DIR}/example/example1.alef)
endif()
