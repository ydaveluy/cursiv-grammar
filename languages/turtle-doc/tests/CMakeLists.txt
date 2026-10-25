# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The upstream corpus for this variant is empty: both documents here are
# written for this port, under tests/ (never example/).
add_executable(turtledoc_grammar_test grammar_test.cpp)
target_link_libraries(turtledoc_grammar_test PRIVATE turtledocCore)
target_compile_definitions(turtledoc_grammar_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME turtledoc_grammar_test COMMAND turtledoc_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET turtledoc_cli)
  add_test(NAME turtledoc_cli_test COMMAND turtledoc_cli
           ${CMAKE_CURRENT_SOURCE_DIR}/features.ttl)
endif()
