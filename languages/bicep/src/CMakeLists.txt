# Generate the parser and its language module.
cursiv_generate_language(ID bicep
    GRAMMAR    bicep/core/BicepGrammar.hpp
    NAMESPACE  bicep::Bicep
    EXTENSIONS .bicep
    OUT        bicep/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bicep/core/*.cpp)
add_library(bicepCore ${_core})
target_link_libraries(bicepCore PUBLIC bicepGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bicep/cli/*.cpp)
if(_cli)
  add_executable(bicep_cli ${_cli})
  target_link_libraries(bicep_cli
                        PRIVATE bicepCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bicep/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/bicep/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/bicep/lsp/main.cpp)
  add_library(bicepLsp ${_lsp})
  target_link_libraries(bicepLsp PUBLIC bicepCore cursiv::lsp)
  add_executable(bicep_lsp bicep/lsp/main.cpp)
  target_link_libraries(bicep_lsp PRIVATE bicepLsp)
endif()

if(TARGET bicepLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bicep/bench/main.cpp")
  cursiv_add_benchmark(bicep
      SOURCES bicep/bench/main.cpp)
endif()

cursiv_package_language(bicep)
