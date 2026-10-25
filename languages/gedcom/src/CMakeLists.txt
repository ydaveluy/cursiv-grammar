# Generate the parser and its language module.
cursiv_generate_language(ID gedcom
    GRAMMAR    gedcom/core/GedcomGrammar.hpp
    NAMESPACE  gedcom::Gedcom
    EXTENSIONS .ged
    OUT        gedcom/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gedcom/core/*.cpp)
add_library(gedcomCore ${_core})
target_link_libraries(gedcomCore PUBLIC gedcomGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gedcom/cli/*.cpp)
if(_cli)
  add_executable(gedcom_cli ${_cli})
  target_link_libraries(gedcom_cli
                        PRIVATE gedcomCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/gedcom/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/gedcom/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/gedcom/lsp/main.cpp)
  add_library(gedcomLsp ${_lsp})
  target_link_libraries(gedcomLsp PUBLIC gedcomCore cursiv::lsp)
  add_executable(gedcom_lsp gedcom/lsp/main.cpp)
  target_link_libraries(gedcom_lsp PRIVATE gedcomLsp)
endif()

if(TARGET gedcomLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gedcom/bench/main.cpp")
  cursiv_add_benchmark(gedcom
      SOURCES gedcom/bench/main.cpp)
endif()

cursiv_package_language(gedcom)
