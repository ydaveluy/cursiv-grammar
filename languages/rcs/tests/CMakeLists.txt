# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every sample under tests/ parses clean.
add_executable(rcs_grammar_test grammar_test.cpp)
target_link_libraries(rcs_grammar_test PRIVATE rcsCore)
target_compile_definitions(rcs_grammar_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME rcs_grammar_test COMMAND rcs_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET rcs_cli)
  add_test(NAME rcs_cli_test COMMAND rcs_cli
           ${CMAKE_CURRENT_SOURCE_DIR}/full.rcs)
endif()

add_executable(rcs_native_test native_test.cpp)
target_link_libraries(rcs_native_test PRIVATE rcsCore cursiv::test)
target_compile_definitions(rcs_native_test PRIVATE CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME rcs_native_test COMMAND rcs_native_test)
