# Generate the parser and its language module.
cursiv_generate_language(ID codeql
    GRAMMAR    codeql/core/CodeqlGrammar.hpp
    NAMESPACE  codeql::Codeql
    EXTENSIONS .ql .qll
    OUT        codeql/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/codeql/core/*.cpp)
add_library(codeqlCore ${_core})
target_link_libraries(codeqlCore PUBLIC codeqlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/codeql/cli/*.cpp)
if(_cli)
  add_executable(codeql_cli ${_cli})
  target_link_libraries(codeql_cli
                        PRIVATE codeqlCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/codeql/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/codeql/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/codeql/lsp/main.cpp)
  add_library(codeqlLsp ${_lsp})
  target_link_libraries(codeqlLsp PUBLIC codeqlCore cursiv::lsp)
  add_executable(codeql_lsp codeql/lsp/main.cpp)
  target_link_libraries(codeql_lsp PRIVATE codeqlLsp)
endif()

if(TARGET codeqlLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/codeql/bench/main.cpp")
  cursiv_add_benchmark(codeql
      SOURCES codeql/bench/main.cpp)
endif()

cursiv_package_language(codeql)
