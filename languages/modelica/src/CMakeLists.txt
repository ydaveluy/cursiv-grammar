# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID modelica
    GRAMMAR    modelica/ModelicaGrammar.hpp
    NAMESPACE  modelica::Modelica
    EXTENSIONS .mo
    OUT        modelica/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(modelicaCore ${_core})
target_link_libraries(modelicaCore PUBLIC modelicaGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(modelica_cli cli.cpp)
  target_include_directories(modelica_cli PRIVATE cli)
  target_link_libraries(modelica_cli PRIVATE modelicaCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(modelicaCli ${_cli})
    target_include_directories(modelicaCli PUBLIC cli)
    target_link_libraries(modelicaCli PUBLIC modelicaCore cursiv::cli)
    target_link_libraries(modelica_cli PRIVATE modelicaCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(modelicaLsp ${_lsp})
  target_include_directories(modelicaLsp PUBLIC lsp)
  target_link_libraries(modelicaLsp PUBLIC modelicaCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(modelica_lsp lsp.cpp)
    target_link_libraries(modelica_lsp PRIVATE modelicaLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET modelicaLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(modelica SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(modelica)
