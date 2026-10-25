# Generate the parser and its language module.
cursiv_generate_language(ID wat
    GRAMMAR    wat/core/WatGrammar.hpp
    NAMESPACE  wat::Wat
    EXTENSIONS .wat
    OUT        wat/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wat/core/*.cpp)
add_library(watCore ${_core})
target_link_libraries(watCore PUBLIC watGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wat/cli/*.cpp)
if(_cli)
  add_executable(wat_cli ${_cli})
  target_link_libraries(wat_cli
                        PRIVATE watCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/wat/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/wat/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/wat/lsp/main.cpp)
  add_library(watLsp ${_lsp})
  target_link_libraries(watLsp PUBLIC watCore cursiv::lsp)
  add_executable(wat_lsp wat/lsp/main.cpp)
  target_link_libraries(wat_lsp PRIVATE watLsp)
endif()

if(TARGET watLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/wat/bench/main.cpp")
  cursiv_add_benchmark(wat
      SOURCES wat/bench/main.cpp)
endif()

cursiv_package_language(wat)
