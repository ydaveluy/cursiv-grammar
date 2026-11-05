# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID xml
    GRAMMAR    xml/XmlGrammar.hpp
    NAMESPACE  xml::Xml
    EXTENSIONS .xml
    OUT        xml/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(xmlCore ${_core})
target_link_libraries(xmlCore PUBLIC xmlGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(xml_cli cli.cpp)
  target_include_directories(xml_cli PRIVATE cli)
  target_link_libraries(xml_cli PRIVATE xmlCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(xmlCli ${_cli})
    target_include_directories(xmlCli PUBLIC cli)
    target_link_libraries(xmlCli PUBLIC xmlCore cursiv::cli)
    target_link_libraries(xml_cli PRIVATE xmlCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(xmlLsp ${_lsp})
  target_include_directories(xmlLsp PUBLIC lsp)
  target_link_libraries(xmlLsp PUBLIC xmlCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(xml_lsp lsp.cpp)
    target_link_libraries(xml_lsp PRIVATE xmlLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET xmlLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(xml SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(xml)
