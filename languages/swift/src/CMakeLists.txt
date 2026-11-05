# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID swift
    GRAMMAR    swift/SwiftGrammar.hpp
    NAMESPACE  swift::Swift
    EXTENSIONS .swift
    OUT        swift/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(swiftCore ${_core})
target_link_libraries(swiftCore PUBLIC swiftGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(swift_cli cli.cpp)
  target_include_directories(swift_cli PRIVATE cli)
  target_link_libraries(swift_cli PRIVATE swiftCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(swiftCli ${_cli})
    target_include_directories(swiftCli PUBLIC cli)
    target_link_libraries(swiftCli PUBLIC swiftCore cursiv::cli)
    target_link_libraries(swift_cli PRIVATE swiftCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(swiftLsp ${_lsp})
  target_include_directories(swiftLsp PUBLIC lsp)
  target_link_libraries(swiftLsp PUBLIC swiftCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(swift_lsp lsp.cpp)
    target_link_libraries(swift_lsp PRIVATE swiftLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET swiftLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(swift SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(swift)
