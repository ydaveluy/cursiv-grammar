# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID sgf
    GRAMMAR    sgf/SgfGrammar.hpp
    NAMESPACE  sgf::Sgf
    EXTENSIONS .sgf
    OUT        sgf/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(sgfCore ${_core})
target_link_libraries(sgfCore PUBLIC sgfGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(sgf_cli cli.cpp)
  target_include_directories(sgf_cli PRIVATE cli)
  target_link_libraries(sgf_cli PRIVATE sgfCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(sgfCli ${_cli})
    target_include_directories(sgfCli PUBLIC cli)
    target_link_libraries(sgfCli PUBLIC sgfCore cursiv::cli)
    target_link_libraries(sgf_cli PRIVATE sgfCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(sgfLsp ${_lsp})
  target_include_directories(sgfLsp PUBLIC lsp)
  target_link_libraries(sgfLsp PUBLIC sgfCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(sgf_lsp lsp.cpp)
    target_link_libraries(sgf_lsp PRIVATE sgfLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET sgfLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(sgf SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(sgf)
