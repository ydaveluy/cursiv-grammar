# Generate the parser and its language module.
cursiv_generate_language(ID thrift
    GRAMMAR    thrift/core/ThriftGrammar.hpp
    NAMESPACE  thrift::Thrift
    EXTENSIONS .thrift
    OUT        thrift/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/thrift/core/*.cpp)
add_library(thriftCore ${_core})
target_link_libraries(thriftCore PUBLIC thriftGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/thrift/cli/*.cpp)
if(_cli)
  add_executable(thrift_cli ${_cli})
  target_link_libraries(thrift_cli
                        PRIVATE thriftCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/thrift/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/thrift/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/thrift/lsp/main.cpp)
  add_library(thriftLsp ${_lsp})
  target_link_libraries(thriftLsp PUBLIC thriftCore cursiv::lsp)
  add_executable(thrift_lsp thrift/lsp/main.cpp)
  target_link_libraries(thrift_lsp PRIVATE thriftLsp)
endif()

if(TARGET thriftLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/thrift/bench/main.cpp")
  cursiv_add_benchmark(thrift
      SOURCES thrift/bench/main.cpp)
endif()

cursiv_package_language(thrift)
