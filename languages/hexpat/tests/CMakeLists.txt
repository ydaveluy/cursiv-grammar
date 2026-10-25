# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses, and one document's fields read
# back what it declares.
add_executable(hexpat_grammar_test grammar_test.cpp)
target_link_libraries(hexpat_grammar_test PRIVATE hexpatCore)
target_compile_definitions(hexpat_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME hexpat_grammar_test COMMAND hexpat_grammar_test)

# ...and the command line reads one example.
if(TARGET hexpat_cli)
  add_test(NAME hexpat_cli_test COMMAND hexpat_cli
           ${PROJECT_SOURCE_DIR}/example/dex.hexpat)
endif()
