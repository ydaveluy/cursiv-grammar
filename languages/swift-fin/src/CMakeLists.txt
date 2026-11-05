# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID swiftfin
    GRAMMAR    swiftfin/SwiftFinGrammar.hpp
    NAMESPACE  swiftfin::SwiftFin
    EXTENSIONS .swiftfin
    OUT        swiftfin/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(swiftfinCore ${_core})
target_link_libraries(swiftfinCore PUBLIC swiftfinGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(swiftfin_cli cli.cpp)
  target_include_directories(swiftfin_cli PRIVATE cli)
  target_link_libraries(swiftfin_cli PRIVATE swiftfinCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(swiftfinCli ${_cli})
    target_include_directories(swiftfinCli PUBLIC cli)
    target_link_libraries(swiftfinCli PUBLIC swiftfinCore cursiv::cli)
    target_link_libraries(swiftfin_cli PRIVATE swiftfinCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(swiftfinLsp ${_lsp})
  target_include_directories(swiftfinLsp PUBLIC lsp)
  target_link_libraries(swiftfinLsp PUBLIC swiftfinCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(swiftfin_lsp lsp.cpp)
    target_link_libraries(swiftfin_lsp PRIVATE swiftfinLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET swiftfinLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(swiftfin SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(swiftfin)
