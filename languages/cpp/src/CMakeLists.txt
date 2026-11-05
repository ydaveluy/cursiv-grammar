# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID cpp
    GRAMMAR    cpp/CppGrammar.hpp
    NAMESPACE  cpp::Cpp
    EXTENSIONS .cpp
    OUT        cpp/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(cppCore ${_core})
target_link_libraries(cppCore PUBLIC cppGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(cpp_cli cli.cpp)
  target_include_directories(cpp_cli PRIVATE cli)
  target_link_libraries(cpp_cli PRIVATE cppCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(cppCli ${_cli})
    target_include_directories(cppCli PUBLIC cli)
    target_link_libraries(cppCli PUBLIC cppCore cursiv::cli)
    target_link_libraries(cpp_cli PRIVATE cppCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(cppLsp ${_lsp})
  target_include_directories(cppLsp PUBLIC lsp)
  target_link_libraries(cppLsp PUBLIC cppCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(cpp_lsp lsp.cpp)
    target_link_libraries(cpp_lsp PRIVATE cppLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET cppLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(cpp SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(cpp)
