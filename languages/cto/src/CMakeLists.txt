# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID cto
    GRAMMAR    cto/CtoGrammar.hpp
    NAMESPACE  cto::Cto
    EXTENSIONS .cto
    OUT        cto/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(ctoCore ${_core})
target_link_libraries(ctoCore PUBLIC ctoGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(cto_cli cli.cpp)
  target_include_directories(cto_cli PRIVATE cli)
  target_link_libraries(cto_cli PRIVATE ctoCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(ctoCli ${_cli})
    target_include_directories(ctoCli PUBLIC cli)
    target_link_libraries(ctoCli PUBLIC ctoCore cursiv::cli)
    target_link_libraries(cto_cli PRIVATE ctoCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(ctoLsp ${_lsp})
  target_include_directories(ctoLsp PUBLIC lsp)
  target_link_libraries(ctoLsp PUBLIC ctoCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(cto_lsp lsp.cpp)
    target_link_libraries(cto_lsp PRIVATE ctoLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET ctoLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(cto SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(cto)
