# Generate the parser and its language module.
cursiv_generate_language(ID istc
    GRAMMAR    istc/core/IstcGrammar.hpp
    NAMESPACE  istc::Istc
    EXTENSIONS .istc
    OUT        istc/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/istc/core/*.cpp)
add_library(istcCore ${_core})
target_link_libraries(istcCore PUBLIC istcGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/istc/cli/*.cpp)
if(_cli)
  add_executable(istc_cli ${_cli})
  target_link_libraries(istc_cli
                        PRIVATE istcCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/istc/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/istc/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/istc/lsp/main.cpp)
  add_library(istcLsp ${_lsp})
  target_link_libraries(istcLsp PUBLIC istcCore cursiv::lsp)
  add_executable(istc_lsp istc/lsp/main.cpp)
  target_link_libraries(istc_lsp PRIVATE istcLsp)
endif()

if(TARGET istcLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/istc/bench/main.cpp")
  cursiv_add_benchmark(istc
      SOURCES istc/bench/main.cpp)
endif()

cursiv_package_language(istc)
