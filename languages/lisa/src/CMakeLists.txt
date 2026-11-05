# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID lisa
    GRAMMAR    lisa/LisaGrammar.hpp
    NAMESPACE  lisa::Lisa
    EXTENSIONS .lisa
    OUT        lisa/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(lisaCore ${_core})
target_link_libraries(lisaCore PUBLIC lisaGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(lisa_cli cli.cpp)
  target_include_directories(lisa_cli PRIVATE cli)
  target_link_libraries(lisa_cli PRIVATE lisaCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(lisaCli ${_cli})
    target_include_directories(lisaCli PUBLIC cli)
    target_link_libraries(lisaCli PUBLIC lisaCore cursiv::cli)
    target_link_libraries(lisa_cli PRIVATE lisaCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(lisaLsp ${_lsp})
  target_include_directories(lisaLsp PUBLIC lsp)
  target_link_libraries(lisaLsp PUBLIC lisaCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(lisa_lsp lsp.cpp)
    target_link_libraries(lisa_lsp PRIVATE lisaLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET lisaLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(lisa SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(lisa)
