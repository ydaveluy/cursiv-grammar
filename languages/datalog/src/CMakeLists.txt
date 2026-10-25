# Generate the parser and its language module.
cursiv_generate_language(ID datalog
    GRAMMAR    datalog/core/DatalogGrammar.hpp
    NAMESPACE  datalog::Datalog
    EXTENSIONS .dl
    OUT        datalog/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/datalog/core/*.cpp)
add_library(datalogCore ${_core})
target_link_libraries(datalogCore PUBLIC datalogGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/datalog/cli/*.cpp)
if(_cli)
  add_executable(datalog_cli ${_cli})
  target_link_libraries(datalog_cli
                        PRIVATE datalogCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/datalog/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/datalog/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/datalog/lsp/main.cpp)
  add_library(datalogLsp ${_lsp})
  target_link_libraries(datalogLsp PUBLIC datalogCore cursiv::lsp)
  add_executable(datalog_lsp datalog/lsp/main.cpp)
  target_link_libraries(datalog_lsp PRIVATE datalogLsp)
endif()

if(TARGET datalogLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/datalog/bench/main.cpp")
  cursiv_add_benchmark(datalog
      SOURCES datalog/bench/main.cpp)
endif()

cursiv_package_language(datalog)
