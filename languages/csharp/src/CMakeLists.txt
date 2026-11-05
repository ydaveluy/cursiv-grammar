# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID csharp
    GRAMMAR    csharp/CSharpGrammar.hpp
    NAMESPACE  csharp::CSharp
    EXTENSIONS .cs
    OUT        csharp/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(csharpCore ${_core})
target_link_libraries(csharpCore PUBLIC csharpGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(csharp_cli cli.cpp)
  target_include_directories(csharp_cli PRIVATE cli)
  target_link_libraries(csharp_cli PRIVATE csharpCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(csharpCli ${_cli})
    target_include_directories(csharpCli PUBLIC cli)
    target_link_libraries(csharpCli PUBLIC csharpCore cursiv::cli)
    target_link_libraries(csharp_cli PRIVATE csharpCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(csharpLsp ${_lsp})
  target_include_directories(csharpLsp PUBLIC lsp)
  target_link_libraries(csharpLsp PUBLIC csharpCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(csharp_lsp lsp.cpp)
    target_link_libraries(csharp_lsp PRIVATE csharpLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET csharpLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(csharp SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(csharp)
