# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The upstream corpus is empty, so every sample lives here rather than
# under `example/`.

add_executable(restructuredtext_grammar_test grammar_test.cpp)
target_link_libraries(restructuredtext_grammar_test PRIVATE restructuredtextCore)
target_compile_definitions(restructuredtext_grammar_test PRIVATE
    CURSIV_SECTIONS_SAMPLE="${CMAKE_CURRENT_SOURCE_DIR}/sections.rst"
    CURSIV_INLINE_SAMPLE="${CMAKE_CURRENT_SOURCE_DIR}/inline.rst")
add_test(NAME restructuredtext_grammar_test COMMAND restructuredtext_grammar_test)

# ...and the command line reads one of the same samples.
if(TARGET restructuredtext_cli)
  add_test(NAME restructuredtext_cli_test COMMAND restructuredtext_cli
           ${CMAKE_CURRENT_SOURCE_DIR}/sections.rst)
endif()
