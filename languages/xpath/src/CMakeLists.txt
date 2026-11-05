# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID xpath
    GRAMMAR    xpath/XpathGrammar.hpp
    NAMESPACE  xpath::Xpath
    EXTENSIONS .xpath
    OUT        xpath/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(xpathCore ${_core})
target_link_libraries(xpathCore PUBLIC xpathGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(xpath_cli cli.cpp)
  target_include_directories(xpath_cli PRIVATE cli)
  target_link_libraries(xpath_cli PRIVATE xpathCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(xpathCli ${_cli})
    target_include_directories(xpathCli PUBLIC cli)
    target_link_libraries(xpathCli PUBLIC xpathCore cursiv::cli)
    target_link_libraries(xpath_cli PRIVATE xpathCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(xpathLsp ${_lsp})
  target_include_directories(xpathLsp PUBLIC lsp)
  target_link_libraries(xpathLsp PUBLIC xpathCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(xpath_lsp lsp.cpp)
    target_link_libraries(xpath_lsp PRIVATE xpathLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET xpathLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(xpath SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(xpath)
