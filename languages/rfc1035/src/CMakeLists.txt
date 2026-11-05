# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID rfc1035
    GRAMMAR    rfc1035/Rfc1035Grammar.hpp
    NAMESPACE  rfc1035::Rfc1035
    EXTENSIONS .rfc1035
    OUT        rfc1035/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(rfc1035Core ${_core})
target_link_libraries(rfc1035Core PUBLIC rfc1035Generated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(rfc1035_cli cli.cpp)
  target_include_directories(rfc1035_cli PRIVATE cli)
  target_link_libraries(rfc1035_cli PRIVATE rfc1035Core cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(rfc1035Cli ${_cli})
    target_include_directories(rfc1035Cli PUBLIC cli)
    target_link_libraries(rfc1035Cli PUBLIC rfc1035Core cursiv::cli)
    target_link_libraries(rfc1035_cli PRIVATE rfc1035Cli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(rfc1035Lsp ${_lsp})
  target_include_directories(rfc1035Lsp PUBLIC lsp)
  target_link_libraries(rfc1035Lsp PUBLIC rfc1035Core cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(rfc1035_lsp lsp.cpp)
    target_link_libraries(rfc1035_lsp PRIVATE rfc1035Lsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET rfc1035Lsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(rfc1035 SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(rfc1035)
