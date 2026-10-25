# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example under example/ parses clean.
add_executable(focal_grammar_test grammar_test.cpp)
target_link_libraries(focal_grammar_test PRIVATE focalCore)
target_compile_definitions(focal_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME focal_grammar_test COMMAND focal_grammar_test)

# ...and the command line reads one of the same examples.
if(TARGET focal_cli)
  add_test(NAME focal_cli_test COMMAND focal_cli
           ${PROJECT_SOURCE_DIR}/example/example1.focal)
endif()
