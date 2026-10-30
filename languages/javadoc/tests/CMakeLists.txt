# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every sample under example/ parses, clean.
add_executable(javadoc_grammar_test grammar_test.cpp)
target_link_libraries(javadoc_grammar_test PRIVATE javadocCore cursiv::test)
target_compile_definitions(javadoc_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME javadoc_grammar_test COMMAND javadoc_grammar_test)

# ...and the command line reads one sample.
if(TARGET javadoc_cli)
  add_test(NAME javadoc_cli_test COMMAND javadoc_cli
           ${PROJECT_SOURCE_DIR}/example/SimpleExample.javadoc)
endif()

add_executable(javadoc_native_values_test native_values_test.cpp)
target_link_libraries(javadoc_native_values_test PRIVATE javadocCore cursiv::test)
add_test(NAME javadoc_native_values_test COMMAND javadoc_native_values_test)
