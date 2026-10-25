# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses clean into a Document.
add_executable(suokif_grammar_test grammar_test.cpp)
target_link_libraries(suokif_grammar_test PRIVATE suokifCore cursiv::test)
target_compile_definitions(suokif_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME suokif_grammar_test COMMAND suokif_grammar_test)

# ...and the command line reads one example.
if(TARGET suokif_cli)
  add_test(NAME suokif_cli_test COMMAND suokif_cli
           ${PROJECT_SOURCE_DIR}/example/example1.kif)
endif()
