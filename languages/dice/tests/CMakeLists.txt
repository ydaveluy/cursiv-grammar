# Unit tests skip Binaryen; CLI tools and benchmarks keep their normal flags.
if(CMAKE_SYSTEM_NAME STREQUAL "WASI")
  add_link_options(--no-wasm-opt)
endif()

# The grammar itself: the sample parses, and the tree holds what it declares.
add_executable(dice_grammar_test grammar_test.cpp)
target_link_libraries(dice_grammar_test PRIVATE diceCore)
target_compile_definitions(dice_grammar_test PRIVATE
    CURSIV_SAMPLE="${CMAKE_CURRENT_SOURCE_DIR}/grouping.dice")
add_test(NAME dice_grammar_test COMMAND dice_grammar_test)

# ...and the command line reads an upstream corpus document.
if(TARGET dice_cli)
  add_test(NAME dice_cli_test COMMAND dice_cli
           ${PROJECT_SOURCE_DIR}/example/arithmetic_dice.dice)
endif()
