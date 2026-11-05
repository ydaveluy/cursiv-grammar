# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID rcs
    GRAMMAR    rcs/RcsGrammar.hpp
    NAMESPACE  rcs::Rcs
    EXTENSIONS .rcs
    OUT        rcs/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(rcsCore ${_core})
target_link_libraries(rcsCore PUBLIC rcsGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(rcs_cli cli.cpp)
  target_include_directories(rcs_cli PRIVATE cli)
  target_link_libraries(rcs_cli PRIVATE rcsCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(rcsCli ${_cli})
    target_include_directories(rcsCli PUBLIC cli)
    target_link_libraries(rcsCli PUBLIC rcsCore cursiv::cli)
    target_link_libraries(rcs_cli PRIVATE rcsCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(rcsLsp ${_lsp})
  target_include_directories(rcsLsp PUBLIC lsp)
  target_link_libraries(rcsLsp PUBLIC rcsCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(rcs_lsp lsp.cpp)
    target_link_libraries(rcs_lsp PRIVATE rcsLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET rcsLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(rcs SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(rcs)
