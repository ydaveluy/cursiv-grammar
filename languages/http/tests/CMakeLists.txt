# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every corpus document parses, one is read back field by
# field, and obs_fold.http (ours, not upstream) exercises header folding.
add_executable(http_grammar_test grammar_test.cpp)
target_link_libraries(http_grammar_test PRIVATE httpCore)
target_compile_definitions(http_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME http_grammar_test COMMAND http_grammar_test)

# ...and the command line reads one document from the same corpus.
if(TARGET http_cli)
  add_test(NAME http_cli_test COMMAND http_cli
           ${PROJECT_SOURCE_DIR}/example/post.http)
endif()
