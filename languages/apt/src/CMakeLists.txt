# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID apt
    GRAMMAR    apt/AptGrammar.hpp
    NAMESPACE  apt::Apt
    EXTENSIONS .list
    OUT        apt/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(aptCore ${_core})
target_link_libraries(aptCore PUBLIC aptGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(apt_cli cli.cpp)
  target_include_directories(apt_cli PRIVATE cli)
  target_link_libraries(apt_cli PRIVATE aptCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(aptCli ${_cli})
    target_include_directories(aptCli PUBLIC cli)
    target_link_libraries(aptCli PUBLIC aptCore cursiv::cli)
    target_link_libraries(apt_cli PRIVATE aptCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(aptLsp ${_lsp})
  target_include_directories(aptLsp PUBLIC lsp)
  target_link_libraries(aptLsp PUBLIC aptCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(apt_lsp lsp.cpp)
    target_link_libraries(apt_lsp PRIVATE aptLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET aptLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(apt SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(apt)
