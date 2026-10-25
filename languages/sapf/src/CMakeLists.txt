# Generate the parser and its language module.
cursiv_generate_language(ID sapf
    GRAMMAR    sapf/core/SapfGrammar.hpp
    NAMESPACE  sapf::Sapf
    EXTENSIONS .sapf
    OUT        sapf/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sapf/core/*.cpp)
add_library(sapfCore ${_core})
target_link_libraries(sapfCore PUBLIC sapfGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sapf/cli/*.cpp)
if(_cli)
  add_executable(sapf_cli ${_cli})
  target_link_libraries(sapf_cli
                        PRIVATE sapfCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/sapf/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/sapf/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/sapf/lsp/main.cpp)
  add_library(sapfLsp ${_lsp})
  target_link_libraries(sapfLsp PUBLIC sapfCore cursiv::lsp)
  add_executable(sapf_lsp sapf/lsp/main.cpp)
  target_link_libraries(sapf_lsp PRIVATE sapfLsp)
endif()

if(TARGET sapfLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sapf/bench/main.cpp")
  cursiv_add_benchmark(sapf
      SOURCES sapf/bench/main.cpp)
endif()

cursiv_package_language(sapf)
