# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID itn
    GRAMMAR    itn/ItnGrammar.hpp
    NAMESPACE  itn::Itn
    EXTENSIONS .itn
    OUT        itn/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(itnCore ${_core})
target_link_libraries(itnCore PUBLIC itnGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(itn_cli cli.cpp)
  target_include_directories(itn_cli PRIVATE cli)
  target_link_libraries(itn_cli PRIVATE itnCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(itnCli ${_cli})
    target_include_directories(itnCli PUBLIC cli)
    target_link_libraries(itnCli PUBLIC itnCore cursiv::cli)
    target_link_libraries(itn_cli PRIVATE itnCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(itnLsp ${_lsp})
  target_include_directories(itnLsp PUBLIC lsp)
  target_link_libraries(itnLsp PUBLIC itnCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(itn_lsp lsp.cpp)
    target_link_libraries(itn_lsp PRIVATE itnLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET itnLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(itn SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(itn)
