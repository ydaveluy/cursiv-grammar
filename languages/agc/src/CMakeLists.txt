# Generate the parser and its language module.
cursiv_generate_language(ID agc
    GRAMMAR    agc/core/AgcGrammar.hpp
    NAMESPACE  agc::Agc
    EXTENSIONS .agc
    OUT        agc/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/agc/core/*.cpp)
add_library(agcCore ${_core})
target_link_libraries(agcCore PUBLIC agcGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/agc/cli/*.cpp)
if(_cli)
  add_executable(agc_cli ${_cli})
  target_link_libraries(agc_cli
                        PRIVATE agcCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/agc/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/agc/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/agc/lsp/main.cpp)
  add_library(agcLsp ${_lsp})
  target_link_libraries(agcLsp PUBLIC agcCore cursiv::lsp)
  add_executable(agc_lsp agc/lsp/main.cpp)
  target_link_libraries(agc_lsp PRIVATE agcLsp)
endif()

if(TARGET agcLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/agc/bench/main.cpp")
  cursiv_add_benchmark(agc
      SOURCES agc/bench/main.cpp)
endif()

cursiv_package_language(agc)
