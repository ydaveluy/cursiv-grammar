# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID blueprint
    GRAMMAR    blueprint/BlueprintGrammar.hpp
    NAMESPACE  blueprint::Blueprint
    EXTENSIONS .bp
    OUT        blueprint/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(blueprintCore ${_core})
target_link_libraries(blueprintCore PUBLIC blueprintGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(blueprint_cli cli.cpp)
  target_include_directories(blueprint_cli PRIVATE cli)
  target_link_libraries(blueprint_cli PRIVATE blueprintCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(blueprintCli ${_cli})
    target_include_directories(blueprintCli PUBLIC cli)
    target_link_libraries(blueprintCli PUBLIC blueprintCore cursiv::cli)
    target_link_libraries(blueprint_cli PRIVATE blueprintCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(blueprintLsp ${_lsp})
  target_include_directories(blueprintLsp PUBLIC lsp)
  target_link_libraries(blueprintLsp PUBLIC blueprintCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(blueprint_lsp lsp.cpp)
    target_link_libraries(blueprint_lsp PRIVATE blueprintLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET blueprintLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(blueprint SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(blueprint)
