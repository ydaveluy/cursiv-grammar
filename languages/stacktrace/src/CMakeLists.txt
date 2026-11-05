# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID stacktrace
    GRAMMAR    stacktrace/StacktraceGrammar.hpp
    NAMESPACE  stacktrace::Stacktrace
    EXTENSIONS .stacktrace
    OUT        stacktrace/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(stacktraceCore ${_core})
target_link_libraries(stacktraceCore PUBLIC stacktraceGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(stacktrace_cli cli.cpp)
  target_include_directories(stacktrace_cli PRIVATE cli)
  target_link_libraries(stacktrace_cli PRIVATE stacktraceCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(stacktraceCli ${_cli})
    target_include_directories(stacktraceCli PUBLIC cli)
    target_link_libraries(stacktraceCli PUBLIC stacktraceCore cursiv::cli)
    target_link_libraries(stacktrace_cli PRIVATE stacktraceCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(stacktraceLsp ${_lsp})
  target_include_directories(stacktraceLsp PUBLIC lsp)
  target_link_libraries(stacktraceLsp PUBLIC stacktraceCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(stacktrace_lsp lsp.cpp)
    target_link_libraries(stacktrace_lsp PRIVATE stacktraceLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET stacktraceLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(stacktrace SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(stacktrace)
