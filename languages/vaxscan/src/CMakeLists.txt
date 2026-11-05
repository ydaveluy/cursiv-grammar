# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID vaxscan
    GRAMMAR    vaxscan/VaxscanGrammar.hpp
    NAMESPACE  vaxscan::Vaxscan
    EXTENSIONS .scn
    OUT        vaxscan/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(vaxscanCore ${_core})
target_link_libraries(vaxscanCore PUBLIC vaxscanGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(vaxscan_cli cli.cpp)
  target_include_directories(vaxscan_cli PRIVATE cli)
  target_link_libraries(vaxscan_cli PRIVATE vaxscanCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(vaxscanCli ${_cli})
    target_include_directories(vaxscanCli PUBLIC cli)
    target_link_libraries(vaxscanCli PUBLIC vaxscanCore cursiv::cli)
    target_link_libraries(vaxscan_cli PRIVATE vaxscanCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(vaxscanLsp ${_lsp})
  target_include_directories(vaxscanLsp PUBLIC lsp)
  target_link_libraries(vaxscanLsp PUBLIC vaxscanCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(vaxscan_lsp lsp.cpp)
    target_link_libraries(vaxscan_lsp PRIVATE vaxscanLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET vaxscanLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(vaxscan SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(vaxscan)
