# Generate the parser and its language module.
cursiv_generate_language(ID clu
    GRAMMAR    clu/core/CluGrammar.hpp
    NAMESPACE  clu::Clu
    EXTENSIONS .clu
    OUT        clu/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/clu/core/*.cpp)
add_library(cluCore ${_core})
target_link_libraries(cluCore PUBLIC cluGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/clu/cli/*.cpp)
if(_cli)
  add_executable(clu_cli ${_cli})
  target_link_libraries(clu_cli
                        PRIVATE cluCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/clu/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/clu/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/clu/lsp/main.cpp)
  add_library(cluLsp ${_lsp})
  target_link_libraries(cluLsp PUBLIC cluCore cursiv::lsp)
  add_executable(clu_lsp clu/lsp/main.cpp)
  target_link_libraries(clu_lsp PRIVATE cluLsp)
endif()

if(TARGET cluLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/clu/bench/main.cpp")
  cursiv_add_benchmark(clu
      SOURCES clu/bench/main.cpp)
endif()

cursiv_package_language(clu)
