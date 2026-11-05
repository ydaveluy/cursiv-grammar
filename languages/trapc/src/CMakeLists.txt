# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID trapc
    GRAMMAR    trapc/TrapcGrammar.hpp
    NAMESPACE  trapc::Trapc
    EXTENSIONS .tc
    OUT        trapc/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(trapcCore ${_core})
target_link_libraries(trapcCore PUBLIC trapcGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(trapc_cli cli.cpp)
  target_include_directories(trapc_cli PRIVATE cli)
  target_link_libraries(trapc_cli PRIVATE trapcCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(trapcCli ${_cli})
    target_include_directories(trapcCli PUBLIC cli)
    target_link_libraries(trapcCli PUBLIC trapcCore cursiv::cli)
    target_link_libraries(trapc_cli PRIVATE trapcCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(trapcLsp ${_lsp})
  target_include_directories(trapcLsp PUBLIC lsp)
  target_link_libraries(trapcLsp PUBLIC trapcCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(trapc_lsp lsp.cpp)
    target_link_libraries(trapc_lsp PRIVATE trapcLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET trapcLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(trapc SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(trapc)
