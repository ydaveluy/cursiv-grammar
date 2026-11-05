# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID alef
    GRAMMAR    alef/AlefGrammar.hpp
    NAMESPACE  alef::Alef
    EXTENSIONS .alef
    OUT        alef/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(alefCore ${_core})
target_link_libraries(alefCore PUBLIC alefGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(alef_cli cli.cpp)
  target_include_directories(alef_cli PRIVATE cli)
  target_link_libraries(alef_cli PRIVATE alefCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(alefCli ${_cli})
    target_include_directories(alefCli PUBLIC cli)
    target_link_libraries(alefCli PUBLIC alefCore cursiv::cli)
    target_link_libraries(alef_cli PRIVATE alefCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(alefLsp ${_lsp})
  target_include_directories(alefLsp PUBLIC lsp)
  target_link_libraries(alefLsp PUBLIC alefCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(alef_lsp lsp.cpp)
    target_link_libraries(alef_lsp PRIVATE alefLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET alefLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(alef SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(alef)
