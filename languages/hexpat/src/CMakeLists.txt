# Generate the parser and its language module.
cursiv_generate_language(ID hexpat
    GRAMMAR    hexpat/core/HexpatGrammar.hpp
    NAMESPACE  hexpat::Hexpat
    EXTENSIONS .hexpat
    OUT        hexpat/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/hexpat/core/*.cpp)
add_library(hexpatCore ${_core})
target_link_libraries(hexpatCore PUBLIC hexpatGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/hexpat/cli/*.cpp)
if(_cli)
  add_executable(hexpat_cli ${_cli})
  target_link_libraries(hexpat_cli
                        PRIVATE hexpatCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/hexpat/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/hexpat/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/hexpat/lsp/main.cpp)
  add_library(hexpatLsp ${_lsp})
  target_link_libraries(hexpatLsp PUBLIC hexpatCore cursiv::lsp)
  add_executable(hexpat_lsp hexpat/lsp/main.cpp)
  target_link_libraries(hexpat_lsp PRIVATE hexpatLsp)
endif()

if(TARGET hexpatLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/hexpat/bench/main.cpp")
  cursiv_add_benchmark(hexpat
      SOURCES hexpat/bench/main.cpp)
endif()

cursiv_package_language(hexpat)
