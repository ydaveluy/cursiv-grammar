# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every sample parses, and the tree holds what it
# declares.
add_executable(prolog_grammar_test grammar_test.cpp)
target_link_libraries(prolog_grammar_test PRIVATE prologCore)
target_compile_definitions(prolog_grammar_test PRIVATE
    CURSIV_EXAMPLE1="${PROJECT_SOURCE_DIR}/example/example1.pl"
    CURSIV_CONSTRUCTS="${CMAKE_CURRENT_SOURCE_DIR}/constructs.pl")
add_test(NAME prolog_grammar_test COMMAND prolog_grammar_test)

# ...and the command line reads the same sample.
if(TARGET prolog_cli)
  add_test(NAME prolog_cli_test COMMAND prolog_cli
           ${PROJECT_SOURCE_DIR}/example/example1.pl)
endif()
