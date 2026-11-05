# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID ada
    GRAMMAR    ada/AdaGrammar.hpp
    NAMESPACE  ada::Ada
    EXTENSIONS .ada
    OUT        ada/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(adaCore ${_core})
target_link_libraries(adaCore PUBLIC adaGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(ada_cli cli.cpp)
  target_include_directories(ada_cli PRIVATE cli)
  target_link_libraries(ada_cli PRIVATE adaCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(adaCli ${_cli})
    target_include_directories(adaCli PUBLIC cli)
    target_link_libraries(adaCli PUBLIC adaCore cursiv::cli)
    target_link_libraries(ada_cli PRIVATE adaCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(adaLsp ${_lsp})
  target_include_directories(adaLsp PUBLIC lsp)
  target_link_libraries(adaLsp PUBLIC adaCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(ada_lsp lsp.cpp)
    target_link_libraries(ada_lsp PRIVATE adaLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET adaLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(ada SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(ada)
