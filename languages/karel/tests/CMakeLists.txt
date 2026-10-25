# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every file under example/ parses, and the tree holds
# what it declares.
add_executable(karel_grammar_test grammar_test.cpp)
target_link_libraries(karel_grammar_test PRIVATE karelCore)
target_compile_definitions(karel_grammar_test PRIVATE
    CURSIV_EXAMPLE1="${PROJECT_SOURCE_DIR}/example/example1.karel"
    CURSIV_PATROL="${CMAKE_CURRENT_SOURCE_DIR}/patrol.karel")
add_test(NAME karel_grammar_test COMMAND karel_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET karel_cli)
  add_test(NAME karel_cli_test COMMAND karel_cli
           ${PROJECT_SOURCE_DIR}/example/example1.karel)
endif()
