# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID aterm
    GRAMMAR    aterm/AtermGrammar.hpp
    NAMESPACE  aterm::Aterm
    EXTENSIONS .aterm
    OUT        aterm/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(atermCore ${_core})
target_link_libraries(atermCore PUBLIC atermGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(aterm_cli cli.cpp)
  target_include_directories(aterm_cli PRIVATE cli)
  target_link_libraries(aterm_cli PRIVATE atermCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(atermCli ${_cli})
    target_include_directories(atermCli PUBLIC cli)
    target_link_libraries(atermCli PUBLIC atermCore cursiv::cli)
    target_link_libraries(aterm_cli PRIVATE atermCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(atermLsp ${_lsp})
  target_include_directories(atermLsp PUBLIC lsp)
  target_link_libraries(atermLsp PUBLIC atermCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(aterm_lsp lsp.cpp)
    target_link_libraries(aterm_lsp PRIVATE atermLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET atermLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(aterm SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(aterm)
