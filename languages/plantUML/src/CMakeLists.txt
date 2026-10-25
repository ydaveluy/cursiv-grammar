# Generate the parser and its language module.
cursiv_generate_language(ID plantuml
    GRAMMAR    plantuml/core/PlantUMLGrammar.hpp
    NAMESPACE  plantuml::PlantUML
    EXTENSIONS .plantuml
    OUT        plantuml/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/plantuml/core/*.cpp)
add_library(plantumlCore ${_core})
target_link_libraries(plantumlCore PUBLIC plantumlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/plantuml/cli/*.cpp)
if(_cli)
  add_executable(plantuml_cli ${_cli})
  target_link_libraries(plantuml_cli
                        PRIVATE plantumlCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/plantuml/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/plantuml/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/plantuml/lsp/main.cpp)
  add_library(plantumlLsp ${_lsp})
  target_link_libraries(plantumlLsp PUBLIC plantumlCore cursiv::lsp)
  add_executable(plantuml_lsp plantuml/lsp/main.cpp)
  target_link_libraries(plantuml_lsp PRIVATE plantumlLsp)
endif()

if(TARGET plantumlLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plantuml/bench/main.cpp")
  cursiv_add_benchmark(plantuml
      SOURCES plantuml/bench/main.cpp)
endif()

cursiv_package_language(plantuml)
