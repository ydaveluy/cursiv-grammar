# Generate the parser and its language module.
# The seven PowerScript source-export kinds this grammar reads (window,
# user object, menu, application, function, structure, query); .srd
# (DataWindow) is a different grammar, ported separately as powerbuilderdw.
cursiv_generate_language(ID powerbuilder
    GRAMMAR    powerbuilder/core/PowerbuilderGrammar.hpp
    NAMESPACE  powerbuilder::Powerbuilder
    EXTENSIONS .srw .sru .srm .sra .srf .srs .srq
    OUT        powerbuilder/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/powerbuilder/core/*.cpp)
add_library(powerbuilderCore ${_core})
target_link_libraries(powerbuilderCore PUBLIC powerbuilderGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/powerbuilder/cli/*.cpp)
if(_cli)
  add_executable(powerbuilder_cli ${_cli})
  target_link_libraries(powerbuilder_cli
                        PRIVATE powerbuilderCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/powerbuilder/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/powerbuilder/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/powerbuilder/lsp/main.cpp)
  add_library(powerbuilderLsp ${_lsp})
  target_link_libraries(powerbuilderLsp PUBLIC powerbuilderCore cursiv::lsp)
  add_executable(powerbuilder_lsp powerbuilder/lsp/main.cpp)
  target_link_libraries(powerbuilder_lsp PRIVATE powerbuilderLsp)
endif()

if(TARGET powerbuilderLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/powerbuilder/bench/main.cpp")
  cursiv_add_benchmark(powerbuilder
      SOURCES powerbuilder/bench/main.cpp)
endif()

cursiv_package_language(powerbuilder)
