# Generate the parser and its language module.
cursiv_generate_language(ID gtin
    GRAMMAR    gtin/core/GtinGrammar.hpp
    NAMESPACE  gtin::Gtin
    EXTENSIONS .gtin
    OUT        gtin/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gtin/core/*.cpp)
add_library(gtinCore ${_core})
target_link_libraries(gtinCore PUBLIC gtinGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gtin/cli/*.cpp)
if(_cli)
  add_executable(gtin_cli ${_cli})
  target_link_libraries(gtin_cli
                        PRIVATE gtinCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/gtin/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/gtin/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/gtin/lsp/main.cpp)
  add_library(gtinLsp ${_lsp})
  target_link_libraries(gtinLsp PUBLIC gtinCore cursiv::lsp)
  add_executable(gtin_lsp gtin/lsp/main.cpp)
  target_link_libraries(gtin_lsp PRIVATE gtinLsp)
endif()

if(TARGET gtinLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gtin/bench/main.cpp")
  cursiv_add_benchmark(gtin
      SOURCES gtin/bench/main.cpp)
endif()

cursiv_package_language(gtin)
