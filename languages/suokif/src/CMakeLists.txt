# Generate the parser and its language module.
cursiv_generate_language(ID suokif
    GRAMMAR    suokif/core/SuokifGrammar.hpp
    NAMESPACE  suokif::Suokif
    EXTENSIONS .kif
    OUT        suokif/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/suokif/core/*.cpp)
add_library(suokifCore ${_core})
target_link_libraries(suokifCore PUBLIC suokifGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/suokif/cli/*.cpp)
if(_cli)
  add_executable(suokif_cli ${_cli})
  target_link_libraries(suokif_cli
                        PRIVATE suokifCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/suokif/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/suokif/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/suokif/lsp/main.cpp)
  add_library(suokifLsp ${_lsp})
  target_link_libraries(suokifLsp PUBLIC suokifCore cursiv::lsp)
  add_executable(suokif_lsp suokif/lsp/main.cpp)
  target_link_libraries(suokif_lsp PRIVATE suokifLsp)
endif()

if(TARGET suokifLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/suokif/bench/main.cpp")
  cursiv_add_benchmark(suokif
      SOURCES suokif/bench/main.cpp)
endif()

cursiv_package_language(suokif)
