# Generate the parser and its language module.
cursiv_generate_language(ID p
    GRAMMAR    p/core/PGrammar.hpp
    NAMESPACE  p::P
    EXTENSIONS .p
    OUT        p/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/p/core/*.cpp)
add_library(pCore ${_core})
target_link_libraries(pCore PUBLIC pGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/p/cli/*.cpp)
if(_cli)
  add_executable(p_cli ${_cli})
  target_link_libraries(p_cli
                        PRIVATE pCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/p/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/p/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/p/lsp/main.cpp)
  add_library(pLsp ${_lsp})
  target_link_libraries(pLsp PUBLIC pCore cursiv::lsp)
  add_executable(p_lsp p/lsp/main.cpp)
  target_link_libraries(p_lsp PRIVATE pLsp)
endif()

if(TARGET pLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/p/bench/main.cpp")
  cursiv_add_benchmark(p
      SOURCES p/bench/main.cpp)
endif()

cursiv_package_language(p)
