# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every file of example/ parses, and one of them is read
# back for its declared values.
add_executable(v_grammar_test grammar_test.cpp)
target_link_libraries(v_grammar_test PRIVATE vCore)
target_compile_definitions(v_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME v_grammar_test COMMAND v_grammar_test)

# ...and the command line reads one of the same samples.
if(TARGET v_cli)
  add_test(NAME v_cli_test COMMAND v_cli
           ${PROJECT_SOURCE_DIR}/example/helloworld.v)
endif()
