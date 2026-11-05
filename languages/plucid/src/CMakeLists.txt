# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID plucid
    GRAMMAR    plucid/PlucidGrammar.hpp
    NAMESPACE  plucid::Plucid
    EXTENSIONS .plu
    OUT        plucid/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(plucidCore ${_core})
target_link_libraries(plucidCore PUBLIC plucidGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(plucid_cli cli.cpp)
  target_include_directories(plucid_cli PRIVATE cli)
  target_link_libraries(plucid_cli PRIVATE plucidCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(plucidCli ${_cli})
    target_include_directories(plucidCli PUBLIC cli)
    target_link_libraries(plucidCli PUBLIC plucidCore cursiv::cli)
    target_link_libraries(plucid_cli PRIVATE plucidCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(plucidLsp ${_lsp})
  target_include_directories(plucidLsp PUBLIC lsp)
  target_link_libraries(plucidLsp PUBLIC plucidCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(plucid_lsp lsp.cpp)
    target_link_libraries(plucid_lsp PRIVATE plucidLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET plucidLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(plucid SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(plucid)
