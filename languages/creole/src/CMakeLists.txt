# Generate the parser and its language module.
cursiv_generate_language(ID creole
    GRAMMAR    creole/core/CreoleGrammar.hpp
    NAMESPACE  creole::Creole
    EXTENSIONS .creole
    OUT        creole/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/creole/core/*.cpp)
add_library(creoleCore ${_core})
target_link_libraries(creoleCore PUBLIC creoleGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/creole/cli/*.cpp)
if(_cli)
  add_executable(creole_cli ${_cli})
  target_link_libraries(creole_cli
                        PRIVATE creoleCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/creole/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/creole/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/creole/lsp/main.cpp)
  add_library(creoleLsp ${_lsp})
  target_link_libraries(creoleLsp PUBLIC creoleCore cursiv::lsp)
  add_executable(creole_lsp creole/lsp/main.cpp)
  target_link_libraries(creole_lsp PRIVATE creoleLsp)
endif()

if(TARGET creoleLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/creole/bench/main.cpp")
  cursiv_add_benchmark(creole
      SOURCES creole/bench/main.cpp)
endif()

cursiv_package_language(creole)
