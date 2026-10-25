# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: no upstream corpus ships with plucid, so every
# document lives here and is read back field by field.
add_executable(plucid_grammar_test grammar_test.cpp)
target_link_libraries(plucid_grammar_test PRIVATE plucidCore)
target_compile_definitions(plucid_grammar_test PRIVATE
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME plucid_grammar_test COMMAND plucid_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET plucid_cli)
  add_test(NAME plucid_cli_test COMMAND plucid_cli
           ${CMAKE_CURRENT_SOURCE_DIR}/arithmetic.plu)
endif()
