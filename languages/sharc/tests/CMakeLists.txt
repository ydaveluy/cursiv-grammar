# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: upstream ships no corpus for SHARC, so every document
# here is this port's own, under tests/.
add_executable(sharc_grammar_test grammar_test.cpp)
target_link_libraries(sharc_grammar_test PRIVATE sharcCore cursiv::test)
target_compile_definitions(sharc_grammar_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME sharc_grammar_test COMMAND sharc_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET sharc_cli)
  add_test(NAME sharc_cli_test COMMAND sharc_cli
           ${PROJECT_SOURCE_DIR}/tests/instructions.dsp)
endif()
