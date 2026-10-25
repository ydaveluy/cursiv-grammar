# Generate the parser and its language module.
cursiv_generate_language(ID pgn
    GRAMMAR    pgn/core/PgnGrammar.hpp
    NAMESPACE  pgn::Pgn
    EXTENSIONS .pgn
    OUT        pgn/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pgn/core/*.cpp)
add_library(pgnCore ${_core})
target_link_libraries(pgnCore PUBLIC pgnGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pgn/cli/*.cpp)
if(_cli)
  add_executable(pgn_cli ${_cli})
  target_link_libraries(pgn_cli
                        PRIVATE pgnCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/pgn/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/pgn/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/pgn/lsp/main.cpp)
  add_library(pgnLsp ${_lsp})
  target_link_libraries(pgnLsp PUBLIC pgnCore cursiv::lsp)
  add_executable(pgn_lsp pgn/lsp/main.cpp)
  target_link_libraries(pgn_lsp PRIVATE pgnLsp)
endif()

if(TARGET pgnLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pgn/bench/main.cpp")
  cursiv_add_benchmark(pgn
      SOURCES pgn/bench/main.cpp)
endif()

cursiv_package_language(pgn)
