# Generate the parser and its language module.
cursiv_generate_language(ID teal
    GRAMMAR    teal/core/TealGrammar.hpp
    NAMESPACE  teal::Teal
    EXTENSIONS .tl
    OUT        teal/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/teal/core/*.cpp)
add_library(tealCore ${_core})
target_link_libraries(tealCore PUBLIC tealGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/teal/cli/*.cpp)
if(_cli)
  add_executable(teal_cli ${_cli})
  target_link_libraries(teal_cli
                        PRIVATE tealCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/teal/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/teal/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/teal/lsp/main.cpp)
  add_library(tealLsp ${_lsp})
  target_link_libraries(tealLsp PUBLIC tealCore cursiv::lsp)
  add_executable(teal_lsp teal/lsp/main.cpp)
  target_link_libraries(teal_lsp PRIVATE tealLsp)
endif()

if(TARGET tealLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/teal/bench/main.cpp")
  cursiv_add_benchmark(teal
      SOURCES teal/bench/main.cpp)
endif()

cursiv_package_language(teal)
