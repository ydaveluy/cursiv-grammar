# Generate the parser and its language module.
cursiv_generate_language(ID properties
    GRAMMAR    properties/core/PropertiesGrammar.hpp
    NAMESPACE  properties::Properties
    EXTENSIONS .properties
    OUT        properties/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/properties/core/*.cpp)
add_library(propertiesCore ${_core})
target_link_libraries(propertiesCore PUBLIC propertiesGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/properties/cli/*.cpp)
if(_cli)
  add_executable(properties_cli ${_cli})
  target_link_libraries(properties_cli
                        PRIVATE propertiesCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/properties/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/properties/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/properties/lsp/main.cpp)
  add_library(propertiesLsp ${_lsp})
  target_link_libraries(propertiesLsp PUBLIC propertiesCore cursiv::lsp)
  add_executable(properties_lsp properties/lsp/main.cpp)
  target_link_libraries(properties_lsp PRIVATE propertiesLsp)
endif()

if(TARGET propertiesLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/properties/bench/main.cpp")
  cursiv_add_benchmark(properties
      SOURCES properties/bench/main.cpp)
endif()

cursiv_package_language(properties)
