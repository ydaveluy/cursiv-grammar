# Generate the parser and its language module.
cursiv_generate_language(ID scotty
    GRAMMAR    scotty/core/ScottyGrammar.hpp
    NAMESPACE  scotty::Scotty
    EXTENSIONS .scotty
    OUT        scotty/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/scotty/core/*.cpp)
add_library(scottyCore ${_core})
target_link_libraries(scottyCore PUBLIC scottyGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/scotty/cli/*.cpp)
if(_cli)
  add_executable(scotty_cli ${_cli})
  target_link_libraries(scotty_cli
                        PRIVATE scottyCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/scotty/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/scotty/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/scotty/lsp/main.cpp)
  add_library(scottyLsp ${_lsp})
  target_link_libraries(scottyLsp PUBLIC scottyCore cursiv::lsp)
  add_executable(scotty_lsp scotty/lsp/main.cpp)
  target_link_libraries(scotty_lsp PRIVATE scottyLsp)
endif()

if(TARGET scottyLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/scotty/bench/main.cpp")
  cursiv_add_benchmark(scotty
      SOURCES scotty/bench/main.cpp)
endif()

cursiv_package_language(scotty)
