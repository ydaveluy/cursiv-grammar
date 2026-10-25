# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `diceGenerated` (the compiled library) and `dice_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID dice
    GRAMMAR    dice/core/DiceGrammar.hpp
    NAMESPACE  dice::Dice
    EXTENSIONS .dice
    OUT        dice/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/dice/core/*.cpp)
add_library(diceCore ${_core})
target_link_libraries(diceCore PUBLIC diceGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/dice/cli/*.cpp)
add_executable(dice_cli ${_cli})
target_link_libraries(dice_cli
                      PRIVATE diceCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/dice/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/dice/lsp/main.cpp)
add_library(diceLsp ${_lsp})
target_link_libraries(diceLsp PUBLIC diceCore cursiv::lsp)
add_executable(dice_lsp dice/lsp/main.cpp)
target_link_libraries(dice_lsp PRIVATE diceLsp)

cursiv_add_benchmark(dice SOURCES dice/bench/main.cpp)

cursiv_package_language(dice)
