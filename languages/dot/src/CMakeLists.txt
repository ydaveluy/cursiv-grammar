# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID dot
    GRAMMAR    dot/DotGrammar.hpp
    NAMESPACE  dot::Dot
    EXTENSIONS .dot
    OUT        dot/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(dotCore ${_core})
target_link_libraries(dotCore PUBLIC dotGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(dot_cli cli.cpp)
  target_include_directories(dot_cli PRIVATE cli)
  target_link_libraries(dot_cli PRIVATE dotCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(dotCli ${_cli})
    target_include_directories(dotCli PUBLIC cli)
    target_link_libraries(dotCli PUBLIC dotCore cursiv::cli)
    target_link_libraries(dot_cli PRIVATE dotCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(dotLsp ${_lsp})
  target_include_directories(dotLsp PUBLIC lsp)
  target_link_libraries(dotLsp PUBLIC dotCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(dot_lsp lsp.cpp)
    target_link_libraries(dot_lsp PRIVATE dotLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET dotLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(dot SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(dot)
