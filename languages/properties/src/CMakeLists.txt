# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID properties
    GRAMMAR    properties/PropertiesGrammar.hpp
    NAMESPACE  properties::Properties
    EXTENSIONS .properties
    OUT        properties/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(propertiesCore ${_core})
target_link_libraries(propertiesCore PUBLIC propertiesGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(properties_cli cli.cpp)
  target_include_directories(properties_cli PRIVATE cli)
  target_link_libraries(properties_cli PRIVATE propertiesCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(propertiesCli ${_cli})
    target_include_directories(propertiesCli PUBLIC cli)
    target_link_libraries(propertiesCli PUBLIC propertiesCore cursiv::cli)
    target_link_libraries(properties_cli PRIVATE propertiesCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(propertiesLsp ${_lsp})
  target_include_directories(propertiesLsp PUBLIC lsp)
  target_link_libraries(propertiesLsp PUBLIC propertiesCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(properties_lsp lsp.cpp)
    target_link_libraries(properties_lsp PRIVATE propertiesLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET propertiesLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(properties SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(properties)
