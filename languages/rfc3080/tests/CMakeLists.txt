# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every sample under tests/ parses clean.
add_executable(rfc3080_grammar_test grammar_test.cpp)
target_link_libraries(rfc3080_grammar_test PRIVATE rfc3080Core cursiv::test)
target_compile_definitions(rfc3080_grammar_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME rfc3080_grammar_test COMMAND rfc3080_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET rfc3080_cli)
  add_test(NAME rfc3080_cli_test COMMAND rfc3080_cli
           ${CMAKE_CURRENT_SOURCE_DIR}/msg.beep)
endif()

if(TARGET rfc3080_cli)
  add_test(NAME rfc3080_invalid_frame_test COMMAND rfc3080_cli
           ${CMAKE_CURRENT_SOURCE_DIR}/invalid-size/msg.beep)
endif()
set_tests_properties(rfc3080_invalid_frame_test PROPERTIES WILL_FAIL TRUE)

if(TARGET rfc3080_cli)
  add_test(NAME rfc3080_embedded_end_test COMMAND rfc3080_cli
           ${CMAKE_CURRENT_SOURCE_DIR}/native-valid.beep)
endif()
