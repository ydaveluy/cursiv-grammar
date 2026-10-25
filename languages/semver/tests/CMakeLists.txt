# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document under example/ parses clean.
add_executable(semver_grammar_test grammar_test.cpp)
target_link_libraries(semver_grammar_test PRIVATE semverCore cursiv::test)
target_compile_definitions(semver_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME semver_grammar_test COMMAND semver_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET semver_cli)
  add_test(NAME semver_cli_test COMMAND semver_cli
           ${PROJECT_SOURCE_DIR}/example/semver-full.semver)
endif()
