# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses clean.
add_executable(morsecode_grammar_test grammar_test.cpp)
target_link_libraries(morsecode_grammar_test PRIVATE morsecodeCore)
target_compile_definitions(morsecode_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME morsecode_grammar_test COMMAND morsecode_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET morsecode_cli)
  add_test(NAME morsecode_cli_test COMMAND morsecode_cli
           ${PROJECT_SOURCE_DIR}/example/SOS.morsecode)
endif()
