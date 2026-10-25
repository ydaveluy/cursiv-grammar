# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every corpus document parses, and the fields it writes hold.
add_executable(amazonstateslanguageintrinsicfunctions_grammar_test grammar_test.cpp)
target_link_libraries(amazonstateslanguageintrinsicfunctions_grammar_test PRIVATE amazonstateslanguageintrinsicfunctionsCore cursiv::test)
target_compile_definitions(amazonstateslanguageintrinsicfunctions_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME amazonstateslanguageintrinsicfunctions_grammar_test COMMAND amazonstateslanguageintrinsicfunctions_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET amazonstateslanguageintrinsicfunctions_cli)
  add_test(NAME amazonstateslanguageintrinsicfunctions_cli_test COMMAND amazonstateslanguageintrinsicfunctions_cli
           ${PROJECT_SOURCE_DIR}/example/intrinsic_states_format.asl-intrinsic)
endif()
