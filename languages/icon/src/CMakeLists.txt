# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID icon
    GRAMMAR    icon/IconGrammar.hpp
    NAMESPACE  icon::Icon
    EXTENSIONS .icn
    OUT        icon/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(iconCore ${_core})
target_link_libraries(iconCore PUBLIC iconGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(icon_cli cli.cpp)
  target_include_directories(icon_cli PRIVATE cli)
  target_link_libraries(icon_cli PRIVATE iconCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(iconCli ${_cli})
    target_include_directories(iconCli PUBLIC cli)
    target_link_libraries(iconCli PUBLIC iconCore cursiv::cli)
    target_link_libraries(icon_cli PRIVATE iconCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(iconLsp ${_lsp})
  target_include_directories(iconLsp PUBLIC lsp)
  target_link_libraries(iconLsp PUBLIC iconCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(icon_lsp lsp.cpp)
    target_link_libraries(icon_lsp PRIVATE iconLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET iconLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(icon SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(icon)
