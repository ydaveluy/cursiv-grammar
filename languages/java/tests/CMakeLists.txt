# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses clean, and the tree holds a
# compilation unit.
add_executable(java_grammar_test grammar_test.cpp)
target_link_libraries(java_grammar_test PRIVATE javaCore cursiv::test)
target_compile_definitions(java_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME java_grammar_test COMMAND java_grammar_test)

# ...and the command line reads one example.
if(TARGET java_cli)
  add_test(NAME java_cli_test COMMAND java_cli
           ${PROJECT_SOURCE_DIR}/example/GenericConstructor.java)
endif()
