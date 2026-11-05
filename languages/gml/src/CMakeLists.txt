# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID gml
    GRAMMAR    gml/GmlGrammar.hpp
    NAMESPACE  gml::Gml
    EXTENSIONS .gml
    OUT        gml/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(gmlCore ${_core})
target_link_libraries(gmlCore PUBLIC gmlGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(gml_cli cli.cpp)
  target_include_directories(gml_cli PRIVATE cli)
  target_link_libraries(gml_cli PRIVATE gmlCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(gmlCli ${_cli})
    target_include_directories(gmlCli PUBLIC cli)
    target_link_libraries(gmlCli PUBLIC gmlCore cursiv::cli)
    target_link_libraries(gml_cli PRIVATE gmlCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(gmlLsp ${_lsp})
  target_include_directories(gmlLsp PUBLIC lsp)
  target_link_libraries(gmlLsp PUBLIC gmlCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(gml_lsp lsp.cpp)
    target_link_libraries(gml_lsp PRIVATE gmlLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET gmlLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(gml SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(gml)
