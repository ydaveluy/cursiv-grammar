# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID bicep
    GRAMMAR    bicep/BicepGrammar.hpp
    NAMESPACE  bicep::Bicep
    EXTENSIONS .bicep
    OUT        bicep/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(bicepCore ${_core})
target_link_libraries(bicepCore PUBLIC bicepGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(bicep_cli cli.cpp)
  target_include_directories(bicep_cli PRIVATE cli)
  target_link_libraries(bicep_cli PRIVATE bicepCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(bicepCli ${_cli})
    target_include_directories(bicepCli PUBLIC cli)
    target_link_libraries(bicepCli PUBLIC bicepCore cursiv::cli)
    target_link_libraries(bicep_cli PRIVATE bicepCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(bicepLsp ${_lsp})
  target_include_directories(bicepLsp PUBLIC lsp)
  target_link_libraries(bicepLsp PUBLIC bicepCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(bicep_lsp lsp.cpp)
    target_link_libraries(bicep_lsp PRIVATE bicepLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET bicepLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(bicep SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(bicep)
