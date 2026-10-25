# Generate the parser and its language module.
cursiv_generate_language(ID xpath
    GRAMMAR    xpath/core/XpathGrammar.hpp
    NAMESPACE  xpath::Xpath
    EXTENSIONS .xpath
    OUT        xpath/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xpath/core/*.cpp)
add_library(xpathCore ${_core})
target_link_libraries(xpathCore PUBLIC xpathGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xpath/cli/*.cpp)
if(_cli)
  add_executable(xpath_cli ${_cli})
  target_link_libraries(xpath_cli
                        PRIVATE xpathCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/xpath/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/xpath/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/xpath/lsp/main.cpp)
  add_library(xpathLsp ${_lsp})
  target_link_libraries(xpathLsp PUBLIC xpathCore cursiv::lsp)
  add_executable(xpath_lsp xpath/lsp/main.cpp)
  target_link_libraries(xpath_lsp PRIVATE xpathLsp)
endif()

if(TARGET xpathLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/xpath/bench/main.cpp")
  cursiv_add_benchmark(xpath
      SOURCES xpath/bench/main.cpp)
endif()

cursiv_package_language(xpath)
