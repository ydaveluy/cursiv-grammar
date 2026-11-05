# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID tnsnames
    GRAMMAR    tnsnames/TnsnamesGrammar.hpp
    NAMESPACE  tnsnames::Tnsnames
    EXTENSIONS .ora
    OUT        tnsnames/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(tnsnamesCore ${_core})
target_link_libraries(tnsnamesCore PUBLIC tnsnamesGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(tnsnames_cli cli.cpp)
  target_include_directories(tnsnames_cli PRIVATE cli)
  target_link_libraries(tnsnames_cli PRIVATE tnsnamesCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(tnsnamesCli ${_cli})
    target_include_directories(tnsnamesCli PUBLIC cli)
    target_link_libraries(tnsnamesCli PUBLIC tnsnamesCore cursiv::cli)
    target_link_libraries(tnsnames_cli PRIVATE tnsnamesCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(tnsnamesLsp ${_lsp})
  target_include_directories(tnsnamesLsp PUBLIC lsp)
  target_link_libraries(tnsnamesLsp PUBLIC tnsnamesCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(tnsnames_lsp lsp.cpp)
    target_link_libraries(tnsnames_lsp PRIVATE tnsnamesLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET tnsnamesLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(tnsnames SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(tnsnames)
