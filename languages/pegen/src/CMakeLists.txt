# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID pegen
    GRAMMAR    pegen/PegenGrammar.hpp
    NAMESPACE  pegen::Pegen
    EXTENSIONS .gram
    OUT        pegen/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(pegenCore ${_core})
target_link_libraries(pegenCore PUBLIC pegenGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(pegen_cli cli.cpp)
  target_include_directories(pegen_cli PRIVATE cli)
  target_link_libraries(pegen_cli PRIVATE pegenCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(pegenCli ${_cli})
    target_include_directories(pegenCli PUBLIC cli)
    target_link_libraries(pegenCli PUBLIC pegenCore cursiv::cli)
    target_link_libraries(pegen_cli PRIVATE pegenCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(pegenLsp ${_lsp})
  target_include_directories(pegenLsp PUBLIC lsp)
  target_link_libraries(pegenLsp PUBLIC pegenCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(pegen_lsp lsp.cpp)
    target_link_libraries(pegen_lsp PRIVATE pegenLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET pegenLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(pegen SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(pegen)
