# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under tests/ parses, and one is read
# back for real field values.
add_executable(argus_grammar_test grammar_test.cpp)
target_link_libraries(argus_grammar_test PRIVATE argusCore)
target_compile_definitions(argus_grammar_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME argus_grammar_test COMMAND argus_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET argus_cli)
  add_test(NAME argus_cli_test COMMAND argus_cli
           ${CMAKE_CURRENT_SOURCE_DIR}/statements.arg)
endif()
