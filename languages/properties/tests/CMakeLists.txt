# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every example parses clean into a Document.
add_executable(properties_grammar_test grammar_test.cpp)
target_link_libraries(properties_grammar_test PRIVATE propertiesCore cursiv::test)
target_compile_definitions(properties_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example")
add_test(NAME properties_grammar_test COMMAND properties_grammar_test)

# ...and the command line reads one example (the one with a .properties
# extension: the cli dispatches by extension, and the corpus's other files
# keep their original .txt extension).
if(TARGET properties_cli)
  add_test(NAME properties_cli_test COMMAND properties_cli
           ${PROJECT_SOURCE_DIR}/example/ebean.properties)
endif()
