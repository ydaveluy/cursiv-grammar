# Generate the parser and its language module.
cursiv_generate_language(ID molecule
    GRAMMAR    molecule/core/MoleculeGrammar.hpp
    NAMESPACE  molecule::Molecule
    EXTENSIONS .mol
    OUT        molecule/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/molecule/core/*.cpp)
add_library(moleculeCore ${_core})
target_link_libraries(moleculeCore PUBLIC moleculeGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/molecule/cli/*.cpp)
if(_cli)
  add_executable(molecule_cli ${_cli})
  target_link_libraries(molecule_cli
                        PRIVATE moleculeCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/molecule/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/molecule/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/molecule/lsp/main.cpp)
  add_library(moleculeLsp ${_lsp})
  target_link_libraries(moleculeLsp PUBLIC moleculeCore cursiv::lsp)
  add_executable(molecule_lsp molecule/lsp/main.cpp)
  target_link_libraries(molecule_lsp PRIVATE moleculeLsp)
endif()

if(TARGET moleculeLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/molecule/bench/main.cpp")
  cursiv_add_benchmark(molecule
      SOURCES molecule/bench/main.cpp)
endif()

cursiv_package_language(molecule)
