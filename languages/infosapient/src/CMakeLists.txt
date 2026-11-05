# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID infosapient
    GRAMMAR    infosapient/InfosapientGrammar.hpp
    NAMESPACE  infosapient::Infosapient
    EXTENSIONS .infosapient
    OUT        infosapient/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(infosapientCore ${_core})
target_link_libraries(infosapientCore PUBLIC infosapientGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(infosapient_cli cli.cpp)
  target_include_directories(infosapient_cli PRIVATE cli)
  target_link_libraries(infosapient_cli PRIVATE infosapientCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(infosapientCli ${_cli})
    target_include_directories(infosapientCli PUBLIC cli)
    target_link_libraries(infosapientCli PUBLIC infosapientCore cursiv::cli)
    target_link_libraries(infosapient_cli PRIVATE infosapientCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(infosapientLsp ${_lsp})
  target_include_directories(infosapientLsp PUBLIC lsp)
  target_link_libraries(infosapientLsp PUBLIC infosapientCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(infosapient_lsp lsp.cpp)
    target_link_libraries(infosapient_lsp PRIVATE infosapientLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET infosapientLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(infosapient SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(infosapient)
