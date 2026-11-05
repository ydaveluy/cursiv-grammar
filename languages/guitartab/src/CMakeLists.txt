# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID guitartab
    GRAMMAR    guitartab/GuitartabGrammar.hpp
    NAMESPACE  guitartab::Guitartab
    EXTENSIONS .guitartab
    OUT        guitartab/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(guitartabCore ${_core})
target_link_libraries(guitartabCore PUBLIC guitartabGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(guitartab_cli cli.cpp)
  target_include_directories(guitartab_cli PRIVATE cli)
  target_link_libraries(guitartab_cli PRIVATE guitartabCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(guitartabCli ${_cli})
    target_include_directories(guitartabCli PUBLIC cli)
    target_link_libraries(guitartabCli PUBLIC guitartabCore cursiv::cli)
    target_link_libraries(guitartab_cli PRIVATE guitartabCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(guitartabLsp ${_lsp})
  target_include_directories(guitartabLsp PUBLIC lsp)
  target_link_libraries(guitartabLsp PUBLIC guitartabCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(guitartab_lsp lsp.cpp)
    target_link_libraries(guitartab_lsp PRIVATE guitartabLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET guitartabLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(guitartab SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(guitartab)
