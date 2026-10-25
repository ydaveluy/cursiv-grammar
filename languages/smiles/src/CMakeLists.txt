# Generate the parser and its language module.
cursiv_generate_language(ID smiles
    GRAMMAR    smiles/core/SmilesGrammar.hpp
    NAMESPACE  smiles::Smiles
    EXTENSIONS .smi
    OUT        smiles/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/smiles/core/*.cpp)
add_library(smilesCore ${_core})
target_link_libraries(smilesCore PUBLIC smilesGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/smiles/cli/*.cpp)
if(_cli)
  add_executable(smiles_cli ${_cli})
  target_link_libraries(smiles_cli
                        PRIVATE smilesCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/smiles/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/smiles/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/smiles/lsp/main.cpp)
  add_library(smilesLsp ${_lsp})
  target_link_libraries(smilesLsp PUBLIC smilesCore cursiv::lsp)
  add_executable(smiles_lsp smiles/lsp/main.cpp)
  target_link_libraries(smiles_lsp PRIVATE smilesLsp)
endif()

if(TARGET smilesLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/smiles/bench/main.cpp")
  cursiv_add_benchmark(smiles
      SOURCES smiles/bench/main.cpp)
endif()

cursiv_package_language(smiles)
