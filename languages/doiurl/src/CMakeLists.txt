# Generate the parser and its language module.
cursiv_generate_language(ID doiurl
    GRAMMAR    doiurl/core/DoiurlGrammar.hpp
    NAMESPACE  doiurl::Doiurl
    EXTENSIONS .doiurl
    OUT        doiurl/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/doiurl/core/*.cpp)
add_library(doiurlCore ${_core})
target_link_libraries(doiurlCore PUBLIC doiurlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/doiurl/cli/*.cpp)
if(_cli)
  add_executable(doiurl_cli ${_cli})
  target_link_libraries(doiurl_cli
                        PRIVATE doiurlCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/doiurl/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/doiurl/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/doiurl/lsp/main.cpp)
  add_library(doiurlLsp ${_lsp})
  target_link_libraries(doiurlLsp PUBLIC doiurlCore cursiv::lsp)
  add_executable(doiurl_lsp doiurl/lsp/main.cpp)
  target_link_libraries(doiurl_lsp PRIVATE doiurlLsp)
endif()

if(TARGET doiurlLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/doiurl/bench/main.cpp")
  cursiv_add_benchmark(doiurl
      SOURCES doiurl/bench/main.cpp)
endif()

cursiv_package_language(doiurl)
