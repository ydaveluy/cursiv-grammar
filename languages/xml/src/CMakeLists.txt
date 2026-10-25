# Generate the parser and its language module.
cursiv_generate_language(ID xml
    GRAMMAR    xml/core/XmlGrammar.hpp
    NAMESPACE  xml::Xml
    EXTENSIONS .xml
    OUT        xml/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xml/core/*.cpp)
add_library(xmlCore ${_core})
target_link_libraries(xmlCore PUBLIC xmlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xml/cli/*.cpp)
if(_cli)
  add_executable(xml_cli ${_cli})
  target_link_libraries(xml_cli
                        PRIVATE xmlCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/xml/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/xml/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/xml/lsp/main.cpp)
  add_library(xmlLsp ${_lsp})
  target_link_libraries(xmlLsp PUBLIC xmlCore cursiv::lsp)
  add_executable(xml_lsp xml/lsp/main.cpp)
  target_link_libraries(xml_lsp PRIVATE xmlLsp)
endif()

if(TARGET xmlLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/xml/bench/main.cpp")
  cursiv_add_benchmark(xml
      SOURCES xml/bench/main.cpp)
endif()

cursiv_package_language(xml)
