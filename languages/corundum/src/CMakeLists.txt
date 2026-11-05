# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID corundum
    GRAMMAR    corundum/CorundumGrammar.hpp
    NAMESPACE  corundum::Corundum
    EXTENSIONS .rb
    OUT        corundum/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(corundumCore ${_core})
target_link_libraries(corundumCore PUBLIC corundumGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(corundum_cli cli.cpp)
  target_include_directories(corundum_cli PRIVATE cli)
  target_link_libraries(corundum_cli PRIVATE corundumCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(corundumCli ${_cli})
    target_include_directories(corundumCli PUBLIC cli)
    target_link_libraries(corundumCli PUBLIC corundumCore cursiv::cli)
    target_link_libraries(corundum_cli PRIVATE corundumCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(corundumLsp ${_lsp})
  target_include_directories(corundumLsp PUBLIC lsp)
  target_link_libraries(corundumLsp PUBLIC corundumCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(corundum_lsp lsp.cpp)
    target_link_libraries(corundum_lsp PRIVATE corundumLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET corundumLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(corundum SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(corundum)
