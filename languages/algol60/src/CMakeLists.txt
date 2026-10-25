# Generate the parser and its language module.
cursiv_generate_language(ID algol60
    GRAMMAR    algol60/core/Algol60Grammar.hpp
    NAMESPACE  algol60::Algol60
    EXTENSIONS .a60 .alg
    OUT        algol60/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/algol60/core/*.cpp)
add_library(algol60Core ${_core})
target_link_libraries(algol60Core PUBLIC algol60Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/algol60/cli/*.cpp)
if(_cli)
  add_executable(algol60_cli ${_cli})
  target_link_libraries(algol60_cli
                        PRIVATE algol60Core cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/algol60/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/algol60/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/algol60/lsp/main.cpp)
  add_library(algol60Lsp ${_lsp})
  target_link_libraries(algol60Lsp PUBLIC algol60Core cursiv::lsp)
  add_executable(algol60_lsp algol60/lsp/main.cpp)
  target_link_libraries(algol60_lsp PRIVATE algol60Lsp)
endif()

if(TARGET algol60Lsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/algol60/bench/main.cpp")
  cursiv_add_benchmark(algol60
      SOURCES algol60/bench/main.cpp)
endif()

cursiv_package_language(algol60)
