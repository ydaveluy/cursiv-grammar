# Generate the parser and its language module.
cursiv_generate_language(ID limbo
    GRAMMAR    limbo/core/LimboGrammar.hpp
    NAMESPACE  limbo::Limbo
    EXTENSIONS .b
    OUT        limbo/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/limbo/core/*.cpp)
add_library(limboCore ${_core})
target_link_libraries(limboCore PUBLIC limboGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/limbo/cli/*.cpp)
if(_cli)
  add_executable(limbo_cli ${_cli})
  target_link_libraries(limbo_cli
                        PRIVATE limboCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/limbo/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/limbo/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/limbo/lsp/main.cpp)
  add_library(limboLsp ${_lsp})
  target_link_libraries(limboLsp PUBLIC limboCore cursiv::lsp)
  add_executable(limbo_lsp limbo/lsp/main.cpp)
  target_link_libraries(limbo_lsp PRIVATE limboLsp)
endif()

if(TARGET limboLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/limbo/bench/main.cpp")
  cursiv_add_benchmark(limbo
      SOURCES limbo/bench/main.cpp)
endif()

cursiv_package_language(limbo)
