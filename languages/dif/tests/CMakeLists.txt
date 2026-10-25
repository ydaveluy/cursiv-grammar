# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses, and each tree holds what it
# declares.
add_executable(dif_grammar_test grammar_test.cpp)
target_link_libraries(dif_grammar_test PRIVATE difCore cursiv::test)
target_compile_definitions(dif_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME dif_grammar_test COMMAND dif_grammar_test)

# ...and the command line reads one of them.
if(TARGET dif_cli)
  add_test(NAME dif_cli_test COMMAND dif_cli
           ${PROJECT_SOURCE_DIR}/example/example1.dif)
endif()
