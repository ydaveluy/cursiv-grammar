# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID typescript
    GRAMMAR    typescript/TypeScriptGrammar.hpp
    NAMESPACE  typescript::TypeScript
    EXTENSIONS .ts
    OUT        typescript/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(typescriptCore ${_core})
target_link_libraries(typescriptCore PUBLIC typescriptGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(typescript_cli cli.cpp)
  target_include_directories(typescript_cli PRIVATE cli)
  target_link_libraries(typescript_cli PRIVATE typescriptCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(typescriptCli ${_cli})
    target_include_directories(typescriptCli PUBLIC cli)
    target_link_libraries(typescriptCli PUBLIC typescriptCore cursiv::cli)
    target_link_libraries(typescript_cli PRIVATE typescriptCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(typescriptLsp ${_lsp})
  target_include_directories(typescriptLsp PUBLIC lsp)
  target_link_libraries(typescriptLsp PUBLIC typescriptCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(typescript_lsp lsp.cpp)
    target_link_libraries(typescript_lsp PRIVATE typescriptLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET typescriptLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(typescript SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(typescript)
