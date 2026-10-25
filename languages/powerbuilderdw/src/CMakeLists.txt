# Generate the parser and its language module.
cursiv_generate_language(ID powerbuilderdw
    GRAMMAR    powerbuilderdw/core/PowerbuilderdwGrammar.hpp
    NAMESPACE  powerbuilderdw::Powerbuilderdw
    EXTENSIONS .srd
    OUT        powerbuilderdw/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/powerbuilderdw/core/*.cpp)
add_library(powerbuilderdwCore ${_core})
target_link_libraries(powerbuilderdwCore PUBLIC powerbuilderdwGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/powerbuilderdw/cli/*.cpp)
if(_cli)
  add_executable(powerbuilderdw_cli ${_cli})
  target_link_libraries(powerbuilderdw_cli
                        PRIVATE powerbuilderdwCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/powerbuilderdw/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/powerbuilderdw/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/powerbuilderdw/lsp/main.cpp)
  add_library(powerbuilderdwLsp ${_lsp})
  target_link_libraries(powerbuilderdwLsp PUBLIC powerbuilderdwCore cursiv::lsp)
  add_executable(powerbuilderdw_lsp powerbuilderdw/lsp/main.cpp)
  target_link_libraries(powerbuilderdw_lsp PRIVATE powerbuilderdwLsp)
endif()

if(TARGET powerbuilderdwLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/powerbuilderdw/bench/main.cpp")
  cursiv_add_benchmark(powerbuilderdw
      SOURCES powerbuilderdw/bench/main.cpp)
endif()

cursiv_package_language(powerbuilderdw)
