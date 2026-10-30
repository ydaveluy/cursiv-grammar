# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses clean.
add_executable(desktopentry_grammar_test grammar_test.cpp)
target_link_libraries(desktopentry_grammar_test PRIVATE desktopentryCore cursiv::test)
target_compile_definitions(desktopentry_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME desktopentry_grammar_test COMMAND desktopentry_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET desktopentry_cli)
  add_test(NAME desktopentry_cli_test COMMAND desktopentry_cli
           ${PROJECT_SOURCE_DIR}/example/htop.desktop)
endif()
