# Generate the parser and its language module.
cursiv_generate_language(ID kquery
    GRAMMAR    kquery/core/KqueryGrammar.hpp
    NAMESPACE  kquery::Kquery
    EXTENSIONS .kquery
    OUT        kquery/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/kquery/core/*.cpp)
add_library(kqueryCore ${_core})
target_link_libraries(kqueryCore PUBLIC kqueryGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/kquery/cli/*.cpp)
if(_cli)
  add_executable(kquery_cli ${_cli})
  target_link_libraries(kquery_cli
                        PRIVATE kqueryCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/kquery/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/kquery/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/kquery/lsp/main.cpp)
  add_library(kqueryLsp ${_lsp})
  target_link_libraries(kqueryLsp PUBLIC kqueryCore cursiv::lsp)
  add_executable(kquery_lsp kquery/lsp/main.cpp)
  target_link_libraries(kquery_lsp PRIVATE kqueryLsp)
endif()

if(TARGET kqueryLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/kquery/bench/main.cpp")
  cursiv_add_benchmark(kquery
      SOURCES kquery/bench/main.cpp)
endif()

cursiv_package_language(kquery)
