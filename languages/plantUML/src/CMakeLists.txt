# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID plantuml
    GRAMMAR    plantuml/PlantUMLGrammar.hpp
    NAMESPACE  plantuml::PlantUML
    EXTENSIONS .plantuml
    OUT        plantuml/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(plantumlCore ${_core})
target_link_libraries(plantumlCore PUBLIC plantumlGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(plantuml_cli cli.cpp)
  target_include_directories(plantuml_cli PRIVATE cli)
  target_link_libraries(plantuml_cli PRIVATE plantumlCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(plantumlCli ${_cli})
    target_include_directories(plantumlCli PUBLIC cli)
    target_link_libraries(plantumlCli PUBLIC plantumlCore cursiv::cli)
    target_link_libraries(plantuml_cli PRIVATE plantumlCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(plantumlLsp ${_lsp})
  target_include_directories(plantumlLsp PUBLIC lsp)
  target_link_libraries(plantumlLsp PUBLIC plantumlCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(plantuml_lsp lsp.cpp)
    target_link_libraries(plantuml_lsp PRIVATE plantumlLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET plantumlLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(plantuml SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(plantuml)
