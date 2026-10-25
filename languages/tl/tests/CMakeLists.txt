# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under tests/ parses clean and builds
# the right root (the upstream corpus is empty).
add_executable(tl_grammar_test grammar_test.cpp)
target_link_libraries(tl_grammar_test PRIVATE tlCore cursiv::test)
target_compile_definitions(tl_grammar_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME tl_grammar_test COMMAND tl_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET tl_cli)
  add_test(NAME tl_cli_test COMMAND tl_cli
           ${CMAKE_CURRENT_SOURCE_DIR}/precedence.tl)
endif()
