# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID lark
    GRAMMAR    lark/LarkGrammar.hpp
    NAMESPACE  lark::Lark
    EXTENSIONS .lark
    OUT        lark/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(larkCore ${_core})
target_link_libraries(larkCore PUBLIC larkGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(lark_cli cli.cpp)
  target_include_directories(lark_cli PRIVATE cli)
  target_link_libraries(lark_cli PRIVATE larkCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(larkCli ${_cli})
    target_include_directories(larkCli PUBLIC cli)
    target_link_libraries(larkCli PUBLIC larkCore cursiv::cli)
    target_link_libraries(lark_cli PRIVATE larkCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(larkLsp ${_lsp})
  target_include_directories(larkLsp PUBLIC lsp)
  target_link_libraries(larkLsp PUBLIC larkCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(lark_lsp lsp.cpp)
    target_link_libraries(lark_lsp PRIVATE larkLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET larkLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(lark SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(lark)
