# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID wavefront
    GRAMMAR    wavefront/WavefrontGrammar.hpp
    NAMESPACE  wavefront::Wavefront
    EXTENSIONS .obj
    OUT        wavefront/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(wavefrontCore ${_core})
target_link_libraries(wavefrontCore PUBLIC wavefrontGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(wavefront_cli cli.cpp)
  target_include_directories(wavefront_cli PRIVATE cli)
  target_link_libraries(wavefront_cli PRIVATE wavefrontCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(wavefrontCli ${_cli})
    target_include_directories(wavefrontCli PUBLIC cli)
    target_link_libraries(wavefrontCli PUBLIC wavefrontCore cursiv::cli)
    target_link_libraries(wavefront_cli PRIVATE wavefrontCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(wavefrontLsp ${_lsp})
  target_include_directories(wavefrontLsp PUBLIC lsp)
  target_link_libraries(wavefrontLsp PUBLIC wavefrontCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(wavefront_lsp lsp.cpp)
    target_link_libraries(wavefront_lsp PRIVATE wavefrontLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET wavefrontLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(wavefront SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(wavefront)
