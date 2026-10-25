# Generate the parser and its language module.
cursiv_generate_language(ID http
    GRAMMAR    http/core/HttpGrammar.hpp
    NAMESPACE  http::Http
    EXTENSIONS .http
    OUT        http/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/http/core/*.cpp)
add_library(httpCore ${_core})
target_link_libraries(httpCore PUBLIC httpGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/http/cli/*.cpp)
if(_cli)
  add_executable(http_cli ${_cli})
  target_link_libraries(http_cli
                        PRIVATE httpCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/http/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/http/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/http/lsp/main.cpp)
  add_library(httpLsp ${_lsp})
  target_link_libraries(httpLsp PUBLIC httpCore cursiv::lsp)
  add_executable(http_lsp http/lsp/main.cpp)
  target_link_libraries(http_lsp PRIVATE httpLsp)
endif()

if(TARGET httpLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/http/bench/main.cpp")
  cursiv_add_benchmark(http
      SOURCES http/bench/main.cpp)
endif()

cursiv_package_language(http)
