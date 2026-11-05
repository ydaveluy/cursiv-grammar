# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID desktopentry
    GRAMMAR    desktopentry/DesktopEntryGrammar.hpp
    NAMESPACE  desktopentry::DesktopEntry
    EXTENSIONS .desktop
    OUT        desktopentry/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(desktopentryCore ${_core})
target_link_libraries(desktopentryCore PUBLIC desktopentryGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(desktopentry_cli cli.cpp)
  target_include_directories(desktopentry_cli PRIVATE cli)
  target_link_libraries(desktopentry_cli PRIVATE desktopentryCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(desktopentryCli ${_cli})
    target_include_directories(desktopentryCli PUBLIC cli)
    target_link_libraries(desktopentryCli PUBLIC desktopentryCore cursiv::cli)
    target_link_libraries(desktopentry_cli PRIVATE desktopentryCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(desktopentryLsp ${_lsp})
  target_include_directories(desktopentryLsp PUBLIC lsp)
  target_link_libraries(desktopentryLsp PUBLIC desktopentryCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(desktopentry_lsp lsp.cpp)
    target_link_libraries(desktopentry_lsp PRIVATE desktopentryLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET desktopentryLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(desktopentry SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(desktopentry)
