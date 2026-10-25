# Generate the parser and its language module.
cursiv_generate_language(ID powerquery
    GRAMMAR    powerquery/core/PowerqueryGrammar.hpp
    NAMESPACE  powerquery::Powerquery
    EXTENSIONS .pq
    OUT        powerquery/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/powerquery/core/*.cpp)
add_library(powerqueryCore ${_core})
target_link_libraries(powerqueryCore PUBLIC powerqueryGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/powerquery/cli/*.cpp)
if(_cli)
  add_executable(powerquery_cli ${_cli})
  target_link_libraries(powerquery_cli
                        PRIVATE powerqueryCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/powerquery/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/powerquery/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/powerquery/lsp/main.cpp)
  add_library(powerqueryLsp ${_lsp})
  target_link_libraries(powerqueryLsp PUBLIC powerqueryCore cursiv::lsp)
  add_executable(powerquery_lsp powerquery/lsp/main.cpp)
  target_link_libraries(powerquery_lsp PRIVATE powerqueryLsp)
endif()

if(TARGET powerqueryLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/powerquery/bench/main.cpp")
  cursiv_add_benchmark(powerquery
      SOURCES powerquery/bench/main.cpp)
endif()

cursiv_package_language(powerquery)
