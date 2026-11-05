# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID miniscript
    GRAMMAR    miniscript/MiniscriptGrammar.hpp
    NAMESPACE  miniscript::Miniscript
    EXTENSIONS .ms
    OUT        miniscript/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(miniscriptCore ${_core})
target_link_libraries(miniscriptCore PUBLIC miniscriptGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(miniscript_cli cli.cpp)
  target_include_directories(miniscript_cli PRIVATE cli)
  target_link_libraries(miniscript_cli PRIVATE miniscriptCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(miniscriptCli ${_cli})
    target_include_directories(miniscriptCli PUBLIC cli)
    target_link_libraries(miniscriptCli PUBLIC miniscriptCore cursiv::cli)
    target_link_libraries(miniscript_cli PRIVATE miniscriptCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(miniscriptLsp ${_lsp})
  target_include_directories(miniscriptLsp PUBLIC lsp)
  target_link_libraries(miniscriptLsp PUBLIC miniscriptCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(miniscript_lsp lsp.cpp)
    target_link_libraries(miniscript_lsp PRIVATE miniscriptLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET miniscriptLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(miniscript SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(miniscript)
