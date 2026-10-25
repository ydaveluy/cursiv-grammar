# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The upstream corpus ships no examples/ at all, so every document lives
# here rather than under example/. Every one parses, and holds the type
# the grammar builds.
add_executable(infosapient_grammar_test grammar_test.cpp)
target_link_libraries(infosapient_grammar_test PRIVATE infosapientCore)
target_compile_definitions(infosapient_grammar_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME infosapient_grammar_test COMMAND infosapient_grammar_test)

# ...and the command line reads one sample of that same corpus.
if(TARGET infosapient_cli)
  add_test(NAME infosapient_cli_test COMMAND infosapient_cli
           ${CMAKE_CURRENT_SOURCE_DIR}/OverheatAlarm.infosapient)
endif()
