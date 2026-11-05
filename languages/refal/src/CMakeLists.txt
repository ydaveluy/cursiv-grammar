# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID refal
    GRAMMAR    refal/RefalGrammar.hpp
    NAMESPACE  refal::Refal
    EXTENSIONS .ref
    OUT        refal/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(refalCore ${_core})
target_link_libraries(refalCore PUBLIC refalGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(refal_cli cli.cpp)
  target_include_directories(refal_cli PRIVATE cli)
  target_link_libraries(refal_cli PRIVATE refalCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(refalCli ${_cli})
    target_include_directories(refalCli PUBLIC cli)
    target_link_libraries(refalCli PUBLIC refalCore cursiv::cli)
    target_link_libraries(refal_cli PRIVATE refalCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(refalLsp ${_lsp})
  target_include_directories(refalLsp PUBLIC lsp)
  target_link_libraries(refalLsp PUBLIC refalCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(refal_lsp lsp.cpp)
    target_link_libraries(refal_lsp PRIVATE refalLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET refalLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(refal SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(refal)
