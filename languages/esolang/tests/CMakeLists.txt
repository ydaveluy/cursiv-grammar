# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: the corpus example and a hand-written document
# exercising every construct of the language.
add_executable(lolcode_grammar_test grammar_test.cpp)
target_link_libraries(lolcode_grammar_test PRIVATE lolcodeCore)
target_compile_definitions(lolcode_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME lolcode_grammar_test COMMAND lolcode_grammar_test)

# ...and the command line reads the corpus sample.
if(TARGET lolcode_cli)
  add_test(NAME lolcode_cli_test COMMAND lolcode_cli
           ${PROJECT_SOURCE_DIR}/example/hai.lol)
endif()
