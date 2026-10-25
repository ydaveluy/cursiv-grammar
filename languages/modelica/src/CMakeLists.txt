# Generate the parser and its language module.
cursiv_generate_language(ID modelica
    GRAMMAR    modelica/core/ModelicaGrammar.hpp
    NAMESPACE  modelica::Modelica
    EXTENSIONS .mo
    OUT        modelica/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/modelica/core/*.cpp)
add_library(modelicaCore ${_core})
target_link_libraries(modelicaCore PUBLIC modelicaGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/modelica/cli/*.cpp)
if(_cli)
  add_executable(modelica_cli ${_cli})
  target_link_libraries(modelica_cli
                        PRIVATE modelicaCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/modelica/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/modelica/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/modelica/lsp/main.cpp)
  add_library(modelicaLsp ${_lsp})
  target_link_libraries(modelicaLsp PUBLIC modelicaCore cursiv::lsp)
  add_executable(modelica_lsp modelica/lsp/main.cpp)
  target_link_libraries(modelica_lsp PRIVATE modelicaLsp)
endif()

if(TARGET modelicaLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modelica/bench/main.cpp")
  cursiv_add_benchmark(modelica
      SOURCES modelica/bench/main.cpp)
endif()

cursiv_package_language(modelica)
