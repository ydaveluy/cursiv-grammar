# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID pascal
    GRAMMAR    pascal/PascalGrammar.hpp
    NAMESPACE  pascal::Pascal
    EXTENSIONS .pas
    OUT        pascal/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(pascalCore ${_core})
target_link_libraries(pascalCore PUBLIC pascalGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(pascal_cli cli.cpp)
  target_include_directories(pascal_cli PRIVATE cli)
  target_link_libraries(pascal_cli PRIVATE pascalCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(pascalCli ${_cli})
    target_include_directories(pascalCli PUBLIC cli)
    target_link_libraries(pascalCli PUBLIC pascalCore cursiv::cli)
    target_link_libraries(pascal_cli PRIVATE pascalCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(pascalLsp ${_lsp})
  target_include_directories(pascalLsp PUBLIC lsp)
  target_link_libraries(pascalLsp PUBLIC pascalCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(pascal_lsp lsp.cpp)
    target_link_libraries(pascal_lsp PRIVATE pascalLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET pascalLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(pascal SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(pascal)
