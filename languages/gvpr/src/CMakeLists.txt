# Generate the parser and its language module.
cursiv_generate_language(ID gvpr
    GRAMMAR    gvpr/core/GvprGrammar.hpp
    NAMESPACE  gvpr::Gvpr
    EXTENSIONS .gvpr
    OUT        gvpr/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gvpr/core/*.cpp)
add_library(gvprCore ${_core})
target_link_libraries(gvprCore PUBLIC gvprGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gvpr/cli/*.cpp)
if(_cli)
  add_executable(gvpr_cli ${_cli})
  target_link_libraries(gvpr_cli
                        PRIVATE gvprCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/gvpr/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/gvpr/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/gvpr/lsp/main.cpp)
  add_library(gvprLsp ${_lsp})
  target_link_libraries(gvprLsp PUBLIC gvprCore cursiv::lsp)
  add_executable(gvpr_lsp gvpr/lsp/main.cpp)
  target_link_libraries(gvpr_lsp PRIVATE gvprLsp)
endif()

if(TARGET gvprLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gvpr/bench/main.cpp")
  cursiv_add_benchmark(gvpr
      SOURCES gvpr/bench/main.cpp)
endif()

cursiv_package_language(gvpr)
