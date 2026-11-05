# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID oberon
    GRAMMAR    oberon/OberonGrammar.hpp
    NAMESPACE  oberon::Oberon
    EXTENSIONS .mod
    OUT        oberon/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(oberonCore ${_core})
target_link_libraries(oberonCore PUBLIC oberonGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(oberon_cli cli.cpp)
  target_include_directories(oberon_cli PRIVATE cli)
  target_link_libraries(oberon_cli PRIVATE oberonCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(oberonCli ${_cli})
    target_include_directories(oberonCli PUBLIC cli)
    target_link_libraries(oberonCli PUBLIC oberonCore cursiv::cli)
    target_link_libraries(oberon_cli PRIVATE oberonCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(oberonLsp ${_lsp})
  target_include_directories(oberonLsp PUBLIC lsp)
  target_link_libraries(oberonLsp PUBLIC oberonCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(oberon_lsp lsp.cpp)
    target_link_libraries(oberon_lsp PRIVATE oberonLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET oberonLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(oberon SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(oberon)
