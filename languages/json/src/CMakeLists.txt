# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID json
    GRAMMAR    json/JsonGrammar.hpp
    NAMESPACE  json::Json
    EXTENSIONS .json
    OUT        json/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(jsonCore ${_core})
target_link_libraries(jsonCore PUBLIC jsonGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(json_cli cli.cpp)
  target_include_directories(json_cli PRIVATE cli)
  target_link_libraries(json_cli PRIVATE jsonCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(jsonCli ${_cli})
    target_include_directories(jsonCli PUBLIC cli)
    target_link_libraries(jsonCli PUBLIC jsonCore cursiv::cli)
    target_link_libraries(json_cli PRIVATE jsonCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(jsonLsp ${_lsp})
  target_include_directories(jsonLsp PUBLIC lsp)
  target_link_libraries(jsonLsp PUBLIC jsonCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(json_lsp lsp.cpp)
    target_link_libraries(json_lsp PRIVATE jsonLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET jsonLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(json SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(json)
