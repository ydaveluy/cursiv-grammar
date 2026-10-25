# Generate the parser and its language module.
cursiv_generate_language(ID fol
    GRAMMAR    fol/core/FolGrammar.hpp
    NAMESPACE  fol::Fol
    EXTENSIONS .fol
    OUT        fol/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fol/core/*.cpp)
add_library(folCore ${_core})
target_link_libraries(folCore PUBLIC folGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fol/cli/*.cpp)
if(_cli)
  add_executable(fol_cli ${_cli})
  target_link_libraries(fol_cli
                        PRIVATE folCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/fol/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/fol/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/fol/lsp/main.cpp)
  add_library(folLsp ${_lsp})
  target_link_libraries(folLsp PUBLIC folCore cursiv::lsp)
  add_executable(fol_lsp fol/lsp/main.cpp)
  target_link_libraries(fol_lsp PRIVATE folLsp)
endif()

if(TARGET folLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/fol/bench/main.cpp")
  cursiv_add_benchmark(fol
      SOURCES fol/bench/main.cpp)
endif()

cursiv_package_language(fol)
