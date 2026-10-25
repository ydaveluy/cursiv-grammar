# Generate the parser and its language module.
cursiv_generate_language(ID memcachedprotocol
    GRAMMAR    memcachedprotocol/core/MemcachedProtocolGrammar.hpp
    NAMESPACE  memcachedprotocol::MemcachedProtocol
    EXTENSIONS .memcachedprotocol
    OUT        memcachedprotocol/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/memcachedprotocol/core/*.cpp)
add_library(memcachedprotocolCore ${_core})
target_link_libraries(memcachedprotocolCore PUBLIC memcachedprotocolGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/memcachedprotocol/cli/*.cpp)
if(_cli)
  add_executable(memcachedprotocol_cli ${_cli})
  target_link_libraries(memcachedprotocol_cli
                        PRIVATE memcachedprotocolCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/memcachedprotocol/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/memcachedprotocol/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/memcachedprotocol/lsp/main.cpp)
  add_library(memcachedprotocolLsp ${_lsp})
  target_link_libraries(memcachedprotocolLsp PUBLIC memcachedprotocolCore cursiv::lsp)
  add_executable(memcachedprotocol_lsp memcachedprotocol/lsp/main.cpp)
  target_link_libraries(memcachedprotocol_lsp PRIVATE memcachedprotocolLsp)
endif()

if(TARGET memcachedprotocolLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/memcachedprotocol/bench/main.cpp")
  cursiv_add_benchmark(memcachedprotocol
      SOURCES memcachedprotocol/bench/main.cpp)
endif()

cursiv_package_language(memcachedprotocol)
