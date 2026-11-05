# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID golang
    GRAMMAR    golang/GolangGrammar.hpp
    NAMESPACE  golang::Golang
    EXTENSIONS .go
    OUT        golang/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(golangCore ${_core})
target_link_libraries(golangCore PUBLIC golangGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(golang_cli cli.cpp)
  target_include_directories(golang_cli PRIVATE cli)
  target_link_libraries(golang_cli PRIVATE golangCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(golangCli ${_cli})
    target_include_directories(golangCli PUBLIC cli)
    target_link_libraries(golangCli PUBLIC golangCore cursiv::cli)
    target_link_libraries(golang_cli PRIVATE golangCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(golangLsp ${_lsp})
  target_include_directories(golangLsp PUBLIC lsp)
  target_link_libraries(golangLsp PUBLIC golangCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(golang_lsp lsp.cpp)
    target_link_libraries(golang_lsp PRIVATE golangLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET golangLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(golang SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(golang)
