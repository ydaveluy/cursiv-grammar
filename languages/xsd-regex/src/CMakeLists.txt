# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID xsdregex
    GRAMMAR    xsdregex/XsdRegexGrammar.hpp
    NAMESPACE  xsdregex::XsdRegex
    EXTENSIONS .xsdregex
    OUT        xsdregex/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(xsdregexCore ${_core})
target_link_libraries(xsdregexCore PUBLIC xsdregexGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(xsdregex_cli cli.cpp)
  target_include_directories(xsdregex_cli PRIVATE cli)
  target_link_libraries(xsdregex_cli PRIVATE xsdregexCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(xsdregexCli ${_cli})
    target_include_directories(xsdregexCli PUBLIC cli)
    target_link_libraries(xsdregexCli PUBLIC xsdregexCore cursiv::cli)
    target_link_libraries(xsdregex_cli PRIVATE xsdregexCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(xsdregexLsp ${_lsp})
  target_include_directories(xsdregexLsp PUBLIC lsp)
  target_link_libraries(xsdregexLsp PUBLIC xsdregexCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(xsdregex_lsp lsp.cpp)
    target_link_libraries(xsdregex_lsp PRIVATE xsdregexLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET xsdregexLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(xsdregex SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(xsdregex)
