# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses clean into a Doc.
add_executable(csv_grammar_test grammar_test.cpp)
target_link_libraries(csv_grammar_test PRIVATE csvCore cursiv::test)
target_compile_definitions(csv_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME csv_grammar_test COMMAND csv_grammar_test)

# ...and the command line reads one example.
if(TARGET csv_cli)
  add_test(NAME csv_cli_test COMMAND csv_cli
           ${PROJECT_SOURCE_DIR}/example/example1.csv)
endif()
