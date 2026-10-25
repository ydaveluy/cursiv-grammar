# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example under example/ parses clean, and one
# document's tree holds what it declares.
add_executable(gvpr_grammar_test grammar_test.cpp)
target_link_libraries(gvpr_grammar_test PRIVATE gvprCore)
target_compile_definitions(gvpr_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME gvpr_grammar_test COMMAND gvpr_grammar_test)

# ...and the command line reads one of the same samples.
if(TARGET gvpr_cli)
  add_test(NAME gvpr_cli_test COMMAND gvpr_cli
           ${PROJECT_SOURCE_DIR}/example/t1.gvpr)
endif()
