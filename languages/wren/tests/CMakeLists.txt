# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses, and a few field values are read
# back out of the AST.
add_executable(wren_grammar_test grammar_test.cpp)
target_link_libraries(wren_grammar_test PRIVATE wrenCore)
target_compile_definitions(wren_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME wren_grammar_test COMMAND wren_grammar_test)

# ...and the command line reads one of the same samples.
if(TARGET wren_cli)
  add_test(NAME wren_cli_test COMMAND wren_cli
           ${PROJECT_SOURCE_DIR}/example/binary_tree.wren)
endif()
