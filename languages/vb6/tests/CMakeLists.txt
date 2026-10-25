# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses clean.
add_executable(vb6_grammar_test grammar_test.cpp)
target_link_libraries(vb6_grammar_test PRIVATE vb6Core)
target_compile_definitions(vb6_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME vb6_grammar_test COMMAND vb6_grammar_test)

# ...and the command line reads one of them.
if(TARGET vb6_cli)
  add_test(NAME vb6_cli_test COMMAND vb6_cli
           ${PROJECT_SOURCE_DIR}/example/calls/Calls.cls)
endif()
