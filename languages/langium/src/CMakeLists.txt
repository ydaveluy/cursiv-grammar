# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID langium
    GRAMMAR    langium/LangiumGrammar.hpp
    NAMESPACE  langium::Langium
    EXTENSIONS .langium
    OUT        langium/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(langiumCore ${_core})
target_link_libraries(langiumCore PUBLIC langiumGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(langium_cli cli.cpp)
  target_include_directories(langium_cli PRIVATE cli)
  target_link_libraries(langium_cli PRIVATE langiumCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(langiumCli ${_cli})
    target_include_directories(langiumCli PUBLIC cli)
    target_link_libraries(langiumCli PUBLIC langiumCore cursiv::cli)
    target_link_libraries(langium_cli PRIVATE langiumCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(langiumLsp ${_lsp})
  target_include_directories(langiumLsp PUBLIC lsp)
  target_link_libraries(langiumLsp PUBLIC langiumCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(langium_lsp lsp.cpp)
    target_link_libraries(langium_lsp PRIVATE langiumLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET langiumLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(langium SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(langium)
