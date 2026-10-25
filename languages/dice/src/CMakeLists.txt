# Generate the parser and its language module.
cursiv_generate_language(ID dice
    GRAMMAR    dice/core/DiceGrammar.hpp
    NAMESPACE  dice::Dice
    EXTENSIONS .dice
    OUT        dice/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/dice/core/*.cpp)
add_library(diceCore ${_core})
target_link_libraries(diceCore PUBLIC diceGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/dice/cli/*.cpp)
if(_cli)
  add_executable(dice_cli ${_cli})
  target_link_libraries(dice_cli
                        PRIVATE diceCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dice/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/dice/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/dice/lsp/main.cpp)
  add_library(diceLsp ${_lsp})
  target_link_libraries(diceLsp PUBLIC diceCore cursiv::lsp)
  add_executable(dice_lsp dice/lsp/main.cpp)
  target_link_libraries(dice_lsp PRIVATE diceLsp)
endif()

if(TARGET diceLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dice/bench/main.cpp")
  cursiv_add_benchmark(dice
      SOURCES dice/bench/main.cpp)
endif()

cursiv_package_language(dice)
