# Generate the parser and its language module.
cursiv_generate_language(ID fen
    GRAMMAR    fen/core/FenGrammar.hpp
    NAMESPACE  fen::Fen
    EXTENSIONS .fen
    OUT        fen/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fen/core/*.cpp)
add_library(fenCore ${_core})
target_link_libraries(fenCore PUBLIC fenGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fen/cli/*.cpp)
if(_cli)
  add_executable(fen_cli ${_cli})
  target_link_libraries(fen_cli
                        PRIVATE fenCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/fen/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/fen/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/fen/lsp/main.cpp)
  add_library(fenLsp ${_lsp})
  target_link_libraries(fenLsp PUBLIC fenCore cursiv::lsp)
  add_executable(fen_lsp fen/lsp/main.cpp)
  target_link_libraries(fen_lsp PRIVATE fenLsp)
endif()

if(TARGET fenLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/fen/bench/main.cpp")
  cursiv_add_benchmark(fen
      SOURCES fen/bench/main.cpp)
endif()

cursiv_package_language(fen)
