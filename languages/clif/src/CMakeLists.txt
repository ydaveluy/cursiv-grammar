# Generate the parser and its language module.
cursiv_generate_language(ID clif
    GRAMMAR    clif/core/ClifGrammar.hpp
    NAMESPACE  clif::Clif
    EXTENSIONS .clif
    OUT        clif/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/clif/core/*.cpp)
add_library(clifCore ${_core})
target_link_libraries(clifCore PUBLIC clifGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/clif/cli/*.cpp)
if(_cli)
  add_executable(clif_cli ${_cli})
  target_link_libraries(clif_cli
                        PRIVATE clifCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/clif/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/clif/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/clif/lsp/main.cpp)
  add_library(clifLsp ${_lsp})
  target_link_libraries(clifLsp PUBLIC clifCore cursiv::lsp)
  add_executable(clif_lsp clif/lsp/main.cpp)
  target_link_libraries(clif_lsp PRIVATE clifLsp)
endif()

if(TARGET clifLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/clif/bench/main.cpp")
  cursiv_add_benchmark(clif
      SOURCES clif/bench/main.cpp)
endif()

cursiv_package_language(clif)
