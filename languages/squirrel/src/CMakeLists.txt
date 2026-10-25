# Generate the parser and its language module.
cursiv_generate_language(ID squirrel
    GRAMMAR    squirrel/core/SquirrelGrammar.hpp
    NAMESPACE  squirrel::Squirrel
    EXTENSIONS .nut
    OUT        squirrel/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/squirrel/core/*.cpp)
add_library(squirrelCore ${_core})
target_link_libraries(squirrelCore PUBLIC squirrelGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/squirrel/cli/*.cpp)
if(_cli)
  add_executable(squirrel_cli ${_cli})
  target_link_libraries(squirrel_cli
                        PRIVATE squirrelCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/squirrel/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/squirrel/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/squirrel/lsp/main.cpp)
  add_library(squirrelLsp ${_lsp})
  target_link_libraries(squirrelLsp PUBLIC squirrelCore cursiv::lsp)
  add_executable(squirrel_lsp squirrel/lsp/main.cpp)
  target_link_libraries(squirrel_lsp PRIVATE squirrelLsp)
endif()

if(TARGET squirrelLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/squirrel/bench/main.cpp")
  cursiv_add_benchmark(squirrel
      SOURCES squirrel/bench/main.cpp)
endif()

cursiv_package_language(squirrel)
