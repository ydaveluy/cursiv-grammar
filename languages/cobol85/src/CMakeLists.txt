# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID cobol85
    GRAMMAR    cobol85/Cobol85Grammar.hpp
    NAMESPACE  cobol85::Cobol85
    EXTENSIONS .cbl
    OUT        cobol85/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(cobol85Core ${_core})
target_link_libraries(cobol85Core PUBLIC cobol85Generated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(cobol85_cli cli.cpp)
  target_include_directories(cobol85_cli PRIVATE cli)
  target_link_libraries(cobol85_cli PRIVATE cobol85Core cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(cobol85Cli ${_cli})
    target_include_directories(cobol85Cli PUBLIC cli)
    target_link_libraries(cobol85Cli PUBLIC cobol85Core cursiv::cli)
    target_link_libraries(cobol85_cli PRIVATE cobol85Cli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(cobol85Lsp ${_lsp})
  target_include_directories(cobol85Lsp PUBLIC lsp)
  target_link_libraries(cobol85Lsp PUBLIC cobol85Core cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(cobol85_lsp lsp.cpp)
    target_link_libraries(cobol85_lsp PRIVATE cobol85Lsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET cobol85Lsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(cobol85 SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(cobol85)
