# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses, and the tree holds what it
# declares.
add_executable(mps_grammar_test grammar_test.cpp)
target_link_libraries(mps_grammar_test PRIVATE mpsCore)
target_compile_definitions(mps_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME mps_grammar_test COMMAND mps_grammar_test)

# ...and the command line reads one of them.
if(TARGET mps_cli)
  add_test(NAME mps_cli_test COMMAND mps_cli
           ${PROJECT_SOURCE_DIR}/example/example1.mps)
endif()
