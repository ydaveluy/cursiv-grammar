# Generate the parser and its language module.
cursiv_generate_language(ID plucid
    GRAMMAR    plucid/core/PlucidGrammar.hpp
    NAMESPACE  plucid::Plucid
    EXTENSIONS .plu
    OUT        plucid/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/plucid/core/*.cpp)
add_library(plucidCore ${_core})
target_link_libraries(plucidCore PUBLIC plucidGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/plucid/cli/*.cpp)
if(_cli)
  add_executable(plucid_cli ${_cli})
  target_link_libraries(plucid_cli
                        PRIVATE plucidCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/plucid/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/plucid/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/plucid/lsp/main.cpp)
  add_library(plucidLsp ${_lsp})
  target_link_libraries(plucidLsp PUBLIC plucidCore cursiv::lsp)
  add_executable(plucid_lsp plucid/lsp/main.cpp)
  target_link_libraries(plucid_lsp PRIVATE plucidLsp)
endif()

if(TARGET plucidLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/plucid/bench/main.cpp")
  cursiv_add_benchmark(plucid
      SOURCES plucid/bench/main.cpp)
endif()

cursiv_package_language(plucid)
