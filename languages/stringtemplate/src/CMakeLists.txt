# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID stringtemplate
    GRAMMAR    stringtemplate/StringtemplateGrammar.hpp
    NAMESPACE  stringtemplate::Stringtemplate
    EXTENSIONS .st
    OUT        stringtemplate/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(stringtemplateCore ${_core})
target_link_libraries(stringtemplateCore PUBLIC stringtemplateGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(stringtemplate_cli cli.cpp)
  target_include_directories(stringtemplate_cli PRIVATE cli)
  target_link_libraries(stringtemplate_cli PRIVATE stringtemplateCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(stringtemplateCli ${_cli})
    target_include_directories(stringtemplateCli PUBLIC cli)
    target_link_libraries(stringtemplateCli PUBLIC stringtemplateCore cursiv::cli)
    target_link_libraries(stringtemplate_cli PRIVATE stringtemplateCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(stringtemplateLsp ${_lsp})
  target_include_directories(stringtemplateLsp PUBLIC lsp)
  target_link_libraries(stringtemplateLsp PUBLIC stringtemplateCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(stringtemplate_lsp lsp.cpp)
    target_link_libraries(stringtemplate_lsp PRIVATE stringtemplateLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET stringtemplateLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(stringtemplate SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(stringtemplate)
