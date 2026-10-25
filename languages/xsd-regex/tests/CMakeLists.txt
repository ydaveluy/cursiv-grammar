# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses clean.
add_executable(xsdregex_grammar_test grammar_test.cpp)
target_link_libraries(xsdregex_grammar_test PRIVATE xsdregexCore)
target_compile_definitions(xsdregex_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME xsdregex_grammar_test COMMAND xsdregex_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET xsdregex_cli)
  add_test(NAME xsdregex_cli_test COMMAND xsdregex_cli
           ${PROJECT_SOURCE_DIR}/example/example-any.xsdregex)
endif()

add_executable(xsdregex_native_test native_test.cpp)
target_link_libraries(xsdregex_native_test PRIVATE xsdregexCore cursiv::test)
target_compile_definitions(xsdregex_native_test PRIVATE CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME xsdregex_native_test COMMAND xsdregex_native_test)
