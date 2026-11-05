# Core: the language itself. Generates the parser from the grammar, again when
# the grammar changes, then builds the services and checks under core/. Every
# other package links it.
cursiv_generate_language(ID cookie
    GRAMMAR    cookie/CookieGrammar.hpp
    NAMESPACE  cookie::Cookie
    EXTENSIONS .cookie
    OUT        cookie/generated)

file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(cookieCore ${_core})
target_link_libraries(cookieCore PUBLIC cookieGenerated)

# CLI: the command line. cli.cpp starts the executable; the code under cli/
# builds into a library that the tests can link. Delete cli.cpp to drop it.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cli.cpp")
  add_executable(cookie_cli cli.cpp)
  target_include_directories(cookie_cli PRIVATE cli)
  target_link_libraries(cookie_cli PRIVATE cookieCore cursiv::cli)
  file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/cli/*.cpp)
  if(_cli)
    add_library(cookieCli ${_cli})
    target_include_directories(cookieCli PUBLIC cli)
    target_link_libraries(cookieCli PUBLIC cookieCore cursiv::cli)
    target_link_libraries(cookie_cli PRIVATE cookieCli)
  endif()
endif()

# LSP: the language server. lsp.cpp starts the executable; the code under lsp/
# builds into a library that the tests and the benchmark can link. Delete lsp.cpp
# and lsp/ to drop it.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
  add_library(cookieLsp ${_lsp})
  target_include_directories(cookieLsp PUBLIC lsp)
  target_link_libraries(cookieLsp PUBLIC cookieCore cursiv::lsp)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lsp.cpp")
    add_executable(cookie_lsp lsp.cpp)
    target_link_libraries(cookie_lsp PRIVATE cookieLsp)
  endif()
endif()

# Benchmark: bench.cpp measures the language through the LSP library. Delete it
# to drop it.
if(TARGET cookieLsp AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench.cpp")
  cursiv_add_benchmark(cookie SOURCES bench.cpp)
endif()

# VS Code: a WASI build copies the language server into vscode/server/.
cursiv_package_language(cookie)
