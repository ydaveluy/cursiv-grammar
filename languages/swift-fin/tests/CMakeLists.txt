# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: every document in example/ parses, and the tree holds
# what it declares.
add_executable(swiftfin_grammar_test grammar_test.cpp)
target_link_libraries(swiftfin_grammar_test PRIVATE swiftfinCore cursiv::test)
target_compile_definitions(swiftfin_grammar_test PRIVATE
    CURSIV_EXAMPLE_DIR="${PROJECT_SOURCE_DIR}/example"
    CURSIV_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME swiftfin_grammar_test COMMAND swiftfin_grammar_test)

# ...and the command line reads one of the same documents.
if(TARGET swiftfin_cli)
  add_test(NAME swiftfin_cli_test COMMAND swiftfin_cli
           ${PROJECT_SOURCE_DIR}/example/complex2.swiftfin)
endif()
