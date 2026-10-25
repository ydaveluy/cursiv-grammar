# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: the upstream corpus (example/) and coverage.edf (ours)
# parse clean, and the tree holds what they declare.
add_executable(edif300_grammar_test grammar_test.cpp)
target_link_libraries(edif300_grammar_test PRIVATE edif300Core)
target_compile_definitions(edif300_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME edif300_grammar_test COMMAND edif300_grammar_test)

# ...and the command line reads one document from the same corpus.
if(TARGET edif300_cli)
  add_test(NAME edif300_cli_test COMMAND edif300_cli
           ${PROJECT_SOURCE_DIR}/example/empty.edf)
endif()
