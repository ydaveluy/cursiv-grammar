# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document parses, and the tree holds what it
# declares.
add_executable(javacc_grammar_test grammar_test.cpp)
target_link_libraries(javacc_grammar_test PRIVATE javaccCore)
target_compile_definitions(javacc_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME javacc_grammar_test COMMAND javacc_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET javacc_cli)
  add_test(NAME javacc_cli_test COMMAND javacc_cli
           ${PROJECT_SOURCE_DIR}/example/Simple1.jj)
endif()
