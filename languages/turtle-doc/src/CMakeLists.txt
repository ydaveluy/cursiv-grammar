# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID turtledoc
    GRAMMAR    turtledoc/TurtleDocGrammar.hpp
    NAMESPACE  turtledoc::TurtleDoc
    EXTENSIONS .ttl
    OUT        turtledoc/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(turtledocCore ${_core})
target_link_libraries(turtledocCore PUBLIC turtledocGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(turtledoc_cli cli.cpp)
  target_include_directories(turtledoc_cli PRIVATE cli)
  target_link_libraries(turtledoc_cli PRIVATE turtledocCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(turtledocCli ${_cli})
    target_include_directories(turtledocCli PUBLIC cli)
    target_link_libraries(turtledocCli PUBLIC turtledocCore cursiv::cli)
    target_link_libraries(turtledoc_cli PRIVATE turtledocCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(turtledocLsp ${_lsp})
  target_include_directories(turtledocLsp PUBLIC lsp)
  target_link_libraries(turtledocLsp PUBLIC turtledocCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(turtledoc_lsp lsp.cpp)
    target_link_libraries(turtledoc_lsp PRIVATE turtledocLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET turtledocLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(turtledoc SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(turtledoc)
