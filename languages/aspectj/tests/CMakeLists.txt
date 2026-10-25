# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every coverage document parses, and the tree holds
# what it declares. There is no upstream corpus, so
# CURSIV_EXAMPLE_DIR is unused here and every document lives under
# CURSIV_TEST_DIR instead.
add_executable(aspectj_grammar_test grammar_test.cpp)
target_link_libraries(aspectj_grammar_test PRIVATE aspectjCore)
target_compile_definitions(aspectj_grammar_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME aspectj_grammar_test COMMAND aspectj_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET aspectj_cli)
  add_test(NAME aspectj_cli_test COMMAND aspectj_cli
           ${CMAKE_CURRENT_SOURCE_DIR}/aspects.aj)
endif()
