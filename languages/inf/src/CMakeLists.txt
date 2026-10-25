# Generate the parser and its language module.
cursiv_generate_language(ID inf
    GRAMMAR    inf/core/InfGrammar.hpp
    NAMESPACE  inf::Inf
    EXTENSIONS .inf
    OUT        inf/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/inf/core/*.cpp)
add_library(infCore ${_core})
target_link_libraries(infCore PUBLIC infGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/inf/cli/*.cpp)
if(_cli)
  add_executable(inf_cli ${_cli})
  target_link_libraries(inf_cli
                        PRIVATE infCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/inf/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/inf/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/inf/lsp/main.cpp)
  add_library(infLsp ${_lsp})
  target_link_libraries(infLsp PUBLIC infCore cursiv::lsp)
  add_executable(inf_lsp inf/lsp/main.cpp)
  target_link_libraries(inf_lsp PRIVATE infLsp)
endif()

if(TARGET infLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/inf/bench/main.cpp")
  cursiv_add_benchmark(inf
      SOURCES inf/bench/main.cpp)
endif()

cursiv_package_language(inf)
