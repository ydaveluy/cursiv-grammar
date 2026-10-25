# Generate the parser and its language module.
cursiv_generate_language(ID gml
    GRAMMAR    gml/core/GmlGrammar.hpp
    NAMESPACE  gml::Gml
    EXTENSIONS .gml
    OUT        gml/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gml/core/*.cpp)
add_library(gmlCore ${_core})
target_link_libraries(gmlCore PUBLIC gmlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gml/cli/*.cpp)
if(_cli)
  add_executable(gml_cli ${_cli})
  target_link_libraries(gml_cli
                        PRIVATE gmlCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/gml/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/gml/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/gml/lsp/main.cpp)
  add_library(gmlLsp ${_lsp})
  target_link_libraries(gmlLsp PUBLIC gmlCore cursiv::lsp)
  add_executable(gml_lsp gml/lsp/main.cpp)
  target_link_libraries(gml_lsp PRIVATE gmlLsp)
endif()

if(TARGET gmlLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gml/bench/main.cpp")
  cursiv_add_benchmark(gml
      SOURCES gml/bench/main.cpp)
endif()

cursiv_package_language(gml)
