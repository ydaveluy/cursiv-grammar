# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under tests/ parses clean.
add_executable(dcm_grammar_test grammar_test.cpp)
target_link_libraries(dcm_grammar_test PRIVATE dcmCore)
target_compile_definitions(dcm_grammar_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME dcm_grammar_test COMMAND dcm_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET dcm_cli)
  add_test(NAME dcm_cli_test COMMAND dcm_cli
           ${CMAKE_CURRENT_SOURCE_DIR}/kennwert.dcm)
endif()
