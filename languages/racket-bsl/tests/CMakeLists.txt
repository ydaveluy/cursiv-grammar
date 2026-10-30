# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every file of example/ parses clean, and the tree
# holds what each sample declares.
add_executable(racketbsl_grammar_test grammar_test.cpp)
target_link_libraries(racketbsl_grammar_test PRIVATE racketbslCore cursiv::test)
target_compile_definitions(racketbsl_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME racketbsl_grammar_test COMMAND racketbsl_grammar_test)

# ...and the command line reads one of the same samples.
if(TARGET racketbsl_cli)
  add_test(NAME racketbsl_cli_test COMMAND racketbsl_cli
           ${PROJECT_SOURCE_DIR}/example/temperature-conversion.rkt)
endif()
