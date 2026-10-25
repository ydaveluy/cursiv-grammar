# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses, and the tree
# holds what it declares.
add_executable(gtin_grammar_test grammar_test.cpp)
target_link_libraries(gtin_grammar_test PRIVATE gtinCore cursiv::test cursiv::lsp)
target_compile_definitions(gtin_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME gtin_grammar_test COMMAND gtin_grammar_test)

# ...and the command line reads one of them.
if(TARGET gtin_cli)
  add_test(NAME gtin_cli_test COMMAND gtin_cli
           ${PROJECT_SOURCE_DIR}/example/gtin13.gtin)
endif()
