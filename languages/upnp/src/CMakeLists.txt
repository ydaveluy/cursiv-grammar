# Generate the parser and its language module.
cursiv_generate_language(ID upnp
    GRAMMAR    upnp/core/UpnpGrammar.hpp
    NAMESPACE  upnp::Upnp
    EXTENSIONS .upnp
    OUT        upnp/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/upnp/core/*.cpp)
add_library(upnpCore ${_core})
target_link_libraries(upnpCore PUBLIC upnpGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/upnp/cli/*.cpp)
if(_cli)
  add_executable(upnp_cli ${_cli})
  target_link_libraries(upnp_cli
                        PRIVATE upnpCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/upnp/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/upnp/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/upnp/lsp/main.cpp)
  add_library(upnpLsp ${_lsp})
  target_link_libraries(upnpLsp PUBLIC upnpCore cursiv::lsp)
  add_executable(upnp_lsp upnp/lsp/main.cpp)
  target_link_libraries(upnp_lsp PRIVATE upnpLsp)
endif()

if(TARGET upnpLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/upnp/bench/main.cpp")
  cursiv_add_benchmark(upnp
      SOURCES upnp/bench/main.cpp)
endif()

cursiv_package_language(upnp)
