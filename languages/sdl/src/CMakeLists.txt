# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID sdl
    GRAMMAR    sdl/SdlGrammar.hpp
    NAMESPACE  sdl::Sdl
    EXTENSIONS .sdl
    OUT        sdl/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(sdlCore ${_core})
target_link_libraries(sdlCore PUBLIC sdlGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(sdl_cli cli.cpp)
  target_include_directories(sdl_cli PRIVATE cli)
  target_link_libraries(sdl_cli PRIVATE sdlCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(sdlCli ${_cli})
    target_include_directories(sdlCli PUBLIC cli)
    target_link_libraries(sdlCli PUBLIC sdlCore cursiv::cli)
    target_link_libraries(sdl_cli PRIVATE sdlCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(sdlLsp ${_lsp})
  target_include_directories(sdlLsp PUBLIC lsp)
  target_link_libraries(sdlLsp PUBLIC sdlCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(sdl_lsp lsp.cpp)
    target_link_libraries(sdl_lsp PRIVATE sdlLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET sdlLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(sdl SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(sdl)
