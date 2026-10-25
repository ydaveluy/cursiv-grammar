# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: what this language takes, and what it refuses.
add_executable(matlab_grammar_test grammar_test.cpp)
target_link_libraries(matlab_grammar_test PRIVATE matlabCore)
target_compile_definitions(matlab_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME matlab_grammar_test COMMAND matlab_grammar_test)

# ...and the command line reads one sample from the same corpus.
if(TARGET matlab_cli)
  add_test(NAME matlab_cli_test COMMAND matlab_cli
           ${PROJECT_SOURCE_DIR}/example/example1.matlab)
endif()
