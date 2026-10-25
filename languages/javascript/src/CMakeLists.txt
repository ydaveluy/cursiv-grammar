# Generate the parser and its language module.
cursiv_generate_language(ID javascript
    GRAMMAR    javascript/core/JavaScriptGrammar.hpp
    NAMESPACE  javascript::JavaScript
    EXTENSIONS .js
    OUT        javascript/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/javascript/core/*.cpp)
add_library(javascriptCore ${_core})
target_link_libraries(javascriptCore PUBLIC javascriptGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/javascript/cli/*.cpp)
if(_cli)
  add_executable(javascript_cli ${_cli})
  target_link_libraries(javascript_cli
                        PRIVATE javascriptCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/javascript/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/javascript/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/javascript/lsp/main.cpp)
  add_library(javascriptLsp ${_lsp})
  target_link_libraries(javascriptLsp PUBLIC javascriptCore cursiv::lsp)
  add_executable(javascript_lsp javascript/lsp/main.cpp)
  target_link_libraries(javascript_lsp PRIVATE javascriptLsp)
endif()

if(TARGET javascriptLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/javascript/bench/main.cpp")
  cursiv_add_benchmark(javascript
      SOURCES javascript/bench/main.cpp)
endif()

cursiv_package_language(javascript)
