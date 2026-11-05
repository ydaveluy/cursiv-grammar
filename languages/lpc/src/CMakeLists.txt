# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID lpc
    GRAMMAR    lpc/LpcGrammar.hpp
    NAMESPACE  lpc::Lpc
    EXTENSIONS .lpc
    OUT        lpc/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(lpcCore ${_core})
target_link_libraries(lpcCore PUBLIC lpcGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(lpc_cli cli.cpp)
  target_include_directories(lpc_cli PRIVATE cli)
  target_link_libraries(lpc_cli PRIVATE lpcCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(lpcCli ${_cli})
    target_include_directories(lpcCli PUBLIC cli)
    target_link_libraries(lpcCli PUBLIC lpcCore cursiv::cli)
    target_link_libraries(lpc_cli PRIVATE lpcCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(lpcLsp ${_lsp})
  target_include_directories(lpcLsp PUBLIC lsp)
  target_link_libraries(lpcLsp PUBLIC lpcCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(lpc_lsp lsp.cpp)
    target_link_libraries(lpc_lsp PRIVATE lpcLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET lpcLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(lpc SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(lpc)
