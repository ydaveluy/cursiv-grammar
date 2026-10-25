# Generate the parser and its language module.
cursiv_generate_language(ID haskell
    GRAMMAR    haskell/core/HaskellGrammar.hpp
    NAMESPACE  haskell::Haskell
    EXTENSIONS .hs
    OUT        haskell/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/haskell/core/*.cpp)
add_library(haskellCore ${_core})
target_link_libraries(haskellCore PUBLIC haskellGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/haskell/cli/*.cpp)
if(_cli)
  add_executable(haskell_cli ${_cli})
  target_link_libraries(haskell_cli
                        PRIVATE haskellCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/haskell/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/haskell/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/haskell/lsp/main.cpp)
  add_library(haskellLsp ${_lsp})
  target_link_libraries(haskellLsp PUBLIC haskellCore cursiv::lsp)
  add_executable(haskell_lsp haskell/lsp/main.cpp)
  target_link_libraries(haskell_lsp PRIVATE haskellLsp)
endif()

if(TARGET haskellLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/haskell/bench/main.cpp")
  cursiv_add_benchmark(haskell
      SOURCES haskell/bench/main.cpp)
endif()

cursiv_package_language(haskell)
