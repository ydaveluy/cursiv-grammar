# Generate the parser and its language module.
cursiv_generate_language(ID url
    GRAMMAR    url/core/UrlGrammar.hpp
    NAMESPACE  url::Url
    EXTENSIONS .url
    OUT        url/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/url/core/*.cpp)
add_library(urlCore ${_core})
target_link_libraries(urlCore PUBLIC urlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/url/cli/*.cpp)
if(_cli)
  add_executable(url_cli ${_cli})
  target_link_libraries(url_cli
                        PRIVATE urlCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/url/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/url/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/url/lsp/main.cpp)
  add_library(urlLsp ${_lsp})
  target_link_libraries(urlLsp PUBLIC urlCore cursiv::lsp)
  add_executable(url_lsp url/lsp/main.cpp)
  target_link_libraries(url_lsp PRIVATE urlLsp)
endif()

if(TARGET urlLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/url/bench/main.cpp")
  cursiv_add_benchmark(url
      SOURCES url/bench/main.cpp)
endif()

cursiv_package_language(url)
