# Generate the parser and its language module.
cursiv_generate_language(ID vaxscan
    GRAMMAR    vaxscan/core/VaxscanGrammar.hpp
    NAMESPACE  vaxscan::Vaxscan
    EXTENSIONS .scn
    OUT        vaxscan/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/vaxscan/core/*.cpp)
add_library(vaxscanCore ${_core})
target_link_libraries(vaxscanCore PUBLIC vaxscanGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/vaxscan/cli/*.cpp)
if(_cli)
  add_executable(vaxscan_cli ${_cli})
  target_link_libraries(vaxscan_cli
                        PRIVATE vaxscanCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/vaxscan/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/vaxscan/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/vaxscan/lsp/main.cpp)
  add_library(vaxscanLsp ${_lsp})
  target_link_libraries(vaxscanLsp PUBLIC vaxscanCore cursiv::lsp)
  add_executable(vaxscan_lsp vaxscan/lsp/main.cpp)
  target_link_libraries(vaxscan_lsp PRIVATE vaxscanLsp)
endif()

if(TARGET vaxscanLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vaxscan/bench/main.cpp")
  cursiv_add_benchmark(vaxscan
      SOURCES vaxscan/bench/main.cpp)
endif()

cursiv_package_language(vaxscan)
