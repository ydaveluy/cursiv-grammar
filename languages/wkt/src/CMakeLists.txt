# Generate the parser and its language module.
cursiv_generate_language(ID wkt
    GRAMMAR    wkt/core/WktGrammar.hpp
    NAMESPACE  wkt::Wkt
    EXTENSIONS .wkt
    OUT        wkt/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wkt/core/*.cpp)
add_library(wktCore ${_core})
target_link_libraries(wktCore PUBLIC wktGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wkt/cli/*.cpp)
if(_cli)
  add_executable(wkt_cli ${_cli})
  target_link_libraries(wkt_cli
                        PRIVATE wktCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/wkt/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/wkt/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/wkt/lsp/main.cpp)
  add_library(wktLsp ${_lsp})
  target_link_libraries(wktLsp PUBLIC wktCore cursiv::lsp)
  add_executable(wkt_lsp wkt/lsp/main.cpp)
  target_link_libraries(wkt_lsp PRIVATE wktLsp)
endif()

if(TARGET wktLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/wkt/bench/main.cpp")
  cursiv_add_benchmark(wkt
      SOURCES wkt/bench/main.cpp)
endif()

cursiv_package_language(wkt)
