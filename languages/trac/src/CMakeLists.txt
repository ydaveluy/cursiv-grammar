# Generate the parser and its language module.
cursiv_generate_language(ID trac
    GRAMMAR    trac/core/TracGrammar.hpp
    NAMESPACE  trac::Trac
    EXTENSIONS .trac
    OUT        trac/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/trac/core/*.cpp)
add_library(tracCore ${_core})
target_link_libraries(tracCore PUBLIC tracGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/trac/cli/*.cpp)
if(_cli)
  add_executable(trac_cli ${_cli})
  target_link_libraries(trac_cli
                        PRIVATE tracCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/trac/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/trac/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/trac/lsp/main.cpp)
  add_library(tracLsp ${_lsp})
  target_link_libraries(tracLsp PUBLIC tracCore cursiv::lsp)
  add_executable(trac_lsp trac/lsp/main.cpp)
  target_link_libraries(trac_lsp PRIVATE tracLsp)
endif()

if(TARGET tracLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/trac/bench/main.cpp")
  cursiv_add_benchmark(trac
      SOURCES trac/bench/main.cpp)
endif()

cursiv_package_language(trac)
