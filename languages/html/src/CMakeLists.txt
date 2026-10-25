# Generate the parser and its language module.
cursiv_generate_language(ID html
    GRAMMAR    html/core/HtmlGrammar.hpp
    NAMESPACE  html::Html
    EXTENSIONS .html
    OUT        html/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/html/core/*.cpp)
add_library(htmlCore ${_core})
target_link_libraries(htmlCore PUBLIC htmlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/html/cli/*.cpp)
if(_cli)
  add_executable(html_cli ${_cli})
  target_link_libraries(html_cli
                        PRIVATE htmlCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/html/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/html/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/html/lsp/main.cpp)
  add_library(htmlLsp ${_lsp})
  target_link_libraries(htmlLsp PUBLIC htmlCore cursiv::lsp)
  add_executable(html_lsp html/lsp/main.cpp)
  target_link_libraries(html_lsp PRIVATE htmlLsp)
endif()

if(TARGET htmlLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/html/bench/main.cpp")
  cursiv_add_benchmark(html
      SOURCES html/bench/main.cpp)
endif()

cursiv_package_language(html)
