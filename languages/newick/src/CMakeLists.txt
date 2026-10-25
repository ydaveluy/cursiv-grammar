# Generate the parser and its language module.
cursiv_generate_language(ID newick
    GRAMMAR    newick/core/NewickGrammar.hpp
    NAMESPACE  newick::Newick
    EXTENSIONS .nwk
    OUT        newick/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/newick/core/*.cpp)
add_library(newickCore ${_core})
target_link_libraries(newickCore PUBLIC newickGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/newick/cli/*.cpp)
if(_cli)
  add_executable(newick_cli ${_cli})
  target_link_libraries(newick_cli
                        PRIVATE newickCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/newick/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/newick/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/newick/lsp/main.cpp)
  add_library(newickLsp ${_lsp})
  target_link_libraries(newickLsp PUBLIC newickCore cursiv::lsp)
  add_executable(newick_lsp newick/lsp/main.cpp)
  target_link_libraries(newick_lsp PRIVATE newickLsp)
endif()

if(TARGET newickLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/newick/bench/main.cpp")
  cursiv_add_benchmark(newick
      SOURCES newick/bench/main.cpp)
endif()

cursiv_package_language(newick)
