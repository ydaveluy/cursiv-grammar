# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID icalendar
    GRAMMAR    icalendar/IcalendarGrammar.hpp
    NAMESPACE  icalendar::Icalendar
    EXTENSIONS .ics
    OUT        icalendar/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(icalendarCore ${_core})
target_link_libraries(icalendarCore PUBLIC icalendarGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(icalendar_cli cli.cpp)
  target_include_directories(icalendar_cli PRIVATE cli)
  target_link_libraries(icalendar_cli PRIVATE icalendarCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(icalendarCli ${_cli})
    target_include_directories(icalendarCli PUBLIC cli)
    target_link_libraries(icalendarCli PUBLIC icalendarCore cursiv::cli)
    target_link_libraries(icalendar_cli PRIVATE icalendarCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(icalendarLsp ${_lsp})
  target_include_directories(icalendarLsp PUBLIC lsp)
  target_link_libraries(icalendarLsp PUBLIC icalendarCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(icalendar_lsp lsp.cpp)
    target_link_libraries(icalendar_lsp PRIVATE icalendarLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET icalendarLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(icalendar SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(icalendar)
