# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID orwell
    GRAMMAR    orwell/OrwellGrammar.hpp
    NAMESPACE  orwell::Orwell
    EXTENSIONS .orwell
    OUT        orwell/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(orwellCore ${_core})
target_link_libraries(orwellCore PUBLIC orwellGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(orwell_cli cli.cpp)
  target_include_directories(orwell_cli PRIVATE cli)
  target_link_libraries(orwell_cli PRIVATE orwellCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(orwellCli ${_cli})
    target_include_directories(orwellCli PUBLIC cli)
    target_link_libraries(orwellCli PUBLIC orwellCore cursiv::cli)
    target_link_libraries(orwell_cli PRIVATE orwellCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(orwellLsp ${_lsp})
  target_include_directories(orwellLsp PUBLIC lsp)
  target_link_libraries(orwellLsp PUBLIC orwellCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(orwell_lsp lsp.cpp)
    target_link_libraries(orwell_lsp PRIVATE orwellLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET orwellLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(orwell SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(orwell)
