# Generate the parser and its language module.
cursiv_generate_language(ID quakemap
    GRAMMAR    quakemap/core/QuakemapGrammar.hpp
    NAMESPACE  quakemap::Quakemap
    EXTENSIONS .map
    OUT        quakemap/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/quakemap/core/*.cpp)
add_library(quakemapCore ${_core})
target_link_libraries(quakemapCore PUBLIC quakemapGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/quakemap/cli/*.cpp)
if(_cli)
  add_executable(quakemap_cli ${_cli})
  target_link_libraries(quakemap_cli
                        PRIVATE quakemapCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/quakemap/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/quakemap/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/quakemap/lsp/main.cpp)
  add_library(quakemapLsp ${_lsp})
  target_link_libraries(quakemapLsp PUBLIC quakemapCore cursiv::lsp)
  add_executable(quakemap_lsp quakemap/lsp/main.cpp)
  target_link_libraries(quakemap_lsp PRIVATE quakemapLsp)
endif()

if(TARGET quakemapLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/quakemap/bench/main.cpp")
  cursiv_add_benchmark(quakemap
      SOURCES quakemap/bench/main.cpp)
endif()

cursiv_package_language(quakemap)
