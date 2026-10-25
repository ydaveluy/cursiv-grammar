# Generate the parser and its language module.
cursiv_generate_language(ID tsv
    GRAMMAR    tsv/core/TsvGrammar.hpp
    NAMESPACE  tsv::Tsv
    EXTENSIONS .tsv
    OUT        tsv/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tsv/core/*.cpp)
add_library(tsvCore ${_core})
target_link_libraries(tsvCore PUBLIC tsvGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tsv/cli/*.cpp)
if(_cli)
  add_executable(tsv_cli ${_cli})
  target_link_libraries(tsv_cli
                        PRIVATE tsvCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tsv/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/tsv/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/tsv/lsp/main.cpp)
  add_library(tsvLsp ${_lsp})
  target_link_libraries(tsvLsp PUBLIC tsvCore cursiv::lsp)
  add_executable(tsv_lsp tsv/lsp/main.cpp)
  target_link_libraries(tsv_lsp PRIVATE tsvLsp)
endif()

if(TARGET tsvLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tsv/bench/main.cpp")
  cursiv_add_benchmark(tsv
      SOURCES tsv/bench/main.cpp)
endif()

cursiv_package_language(tsv)
