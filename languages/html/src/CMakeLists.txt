# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID html
    GRAMMAR    html/HtmlGrammar.hpp
    NAMESPACE  html::Html
    EXTENSIONS .html
    OUT        html/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(htmlCore ${_core})
target_link_libraries(htmlCore PUBLIC htmlGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(html_cli cli.cpp)
  target_include_directories(html_cli PRIVATE cli)
  target_link_libraries(html_cli PRIVATE htmlCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(htmlCli ${_cli})
    target_include_directories(htmlCli PUBLIC cli)
    target_link_libraries(htmlCli PUBLIC htmlCore cursiv::cli)
    target_link_libraries(html_cli PRIVATE htmlCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(htmlLsp ${_lsp})
  target_include_directories(htmlLsp PUBLIC lsp)
  target_link_libraries(htmlLsp PUBLIC htmlCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(html_lsp lsp.cpp)
    target_link_libraries(html_lsp PRIVATE htmlLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET htmlLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(html SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(html)
