# Generate the parser and its language module.
cursiv_generate_language(ID wln
    GRAMMAR    wln/core/WlnGrammar.hpp
    NAMESPACE  wln::Wln
    EXTENSIONS .wln
    OUT        wln/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wln/core/*.cpp)
add_library(wlnCore ${_core})
target_link_libraries(wlnCore PUBLIC wlnGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/wln/cli/*.cpp)
if(_cli)
  add_executable(wln_cli ${_cli})
  target_link_libraries(wln_cli
                        PRIVATE wlnCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/wln/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/wln/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/wln/lsp/main.cpp)
  add_library(wlnLsp ${_lsp})
  target_link_libraries(wlnLsp PUBLIC wlnCore cursiv::lsp)
  add_executable(wln_lsp wln/lsp/main.cpp)
  target_link_libraries(wln_lsp PRIVATE wlnLsp)
endif()

if(TARGET wlnLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/wln/bench/main.cpp")
  cursiv_add_benchmark(wln
      SOURCES wln/bench/main.cpp)
endif()

cursiv_package_language(wln)
