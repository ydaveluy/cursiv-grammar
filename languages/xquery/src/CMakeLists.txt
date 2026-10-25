# Generate the parser and its language module.
cursiv_generate_language(ID xquery
    GRAMMAR    xquery/core/XQueryGrammar.hpp
    NAMESPACE  xquery::XQuery
    EXTENSIONS .xq
    OUT        xquery/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xquery/core/*.cpp)
add_library(xqueryCore ${_core})
target_link_libraries(xqueryCore PUBLIC xqueryGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xquery/cli/*.cpp)
if(_cli)
  add_executable(xquery_cli ${_cli})
  target_link_libraries(xquery_cli
                        PRIVATE xqueryCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/xquery/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/xquery/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/xquery/lsp/main.cpp)
  add_library(xqueryLsp ${_lsp})
  target_link_libraries(xqueryLsp PUBLIC xqueryCore cursiv::lsp)
  add_executable(xquery_lsp xquery/lsp/main.cpp)
  target_link_libraries(xquery_lsp PRIVATE xqueryLsp)
endif()

if(TARGET xqueryLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/xquery/bench/main.cpp")
  cursiv_add_benchmark(xquery
      SOURCES xquery/bench/main.cpp)
endif()

cursiv_package_language(xquery)
