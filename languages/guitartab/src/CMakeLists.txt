# Generate the parser and its language module.
cursiv_generate_language(ID guitartab
    GRAMMAR    guitartab/core/GuitartabGrammar.hpp
    NAMESPACE  guitartab::Guitartab
    EXTENSIONS .guitartab
    OUT        guitartab/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/guitartab/core/*.cpp)
add_library(guitartabCore ${_core})
target_link_libraries(guitartabCore PUBLIC guitartabGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/guitartab/cli/*.cpp)
if(_cli)
  add_executable(guitartab_cli ${_cli})
  target_link_libraries(guitartab_cli
                        PRIVATE guitartabCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/guitartab/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/guitartab/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/guitartab/lsp/main.cpp)
  add_library(guitartabLsp ${_lsp})
  target_link_libraries(guitartabLsp PUBLIC guitartabCore cursiv::lsp)
  add_executable(guitartab_lsp guitartab/lsp/main.cpp)
  target_link_libraries(guitartab_lsp PRIVATE guitartabLsp)
endif()

if(TARGET guitartabLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/guitartab/bench/main.cpp")
  cursiv_add_benchmark(guitartab
      SOURCES guitartab/bench/main.cpp)
endif()

cursiv_package_language(guitartab)
