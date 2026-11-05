# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID logo
    GRAMMAR    logo/LogoGrammar.hpp
    NAMESPACE  logo::Logo
    EXTENSIONS .logo
    OUT        logo/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(logoCore ${_core})
target_link_libraries(logoCore PUBLIC logoGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(logo_cli cli.cpp)
  target_include_directories(logo_cli PRIVATE cli)
  target_link_libraries(logo_cli PRIVATE logoCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(logoCli ${_cli})
    target_include_directories(logoCli PUBLIC cli)
    target_link_libraries(logoCli PUBLIC logoCore cursiv::cli)
    target_link_libraries(logo_cli PRIVATE logoCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(logoLsp ${_lsp})
  target_include_directories(logoLsp PUBLIC lsp)
  target_link_libraries(logoLsp PUBLIC logoCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(logo_lsp lsp.cpp)
    target_link_libraries(logo_lsp PRIVATE logoLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET logoLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(logo SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(logo)
