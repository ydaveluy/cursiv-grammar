# Generate the parser and its language module.
cursiv_generate_language(ID webidl
    GRAMMAR    webidl/core/WebidlGrammar.hpp
    NAMESPACE  webidl::Webidl
    EXTENSIONS .webidl
    OUT        webidl/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/webidl/core/*.cpp)
add_library(webidlCore ${_core})
target_link_libraries(webidlCore PUBLIC webidlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/webidl/cli/*.cpp)
if(_cli)
  add_executable(webidl_cli ${_cli})
  target_link_libraries(webidl_cli
                        PRIVATE webidlCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/webidl/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/webidl/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/webidl/lsp/main.cpp)
  add_library(webidlLsp ${_lsp})
  target_link_libraries(webidlLsp PUBLIC webidlCore cursiv::lsp)
  add_executable(webidl_lsp webidl/lsp/main.cpp)
  target_link_libraries(webidl_lsp PRIVATE webidlLsp)
endif()

if(TARGET webidlLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/webidl/bench/main.cpp")
  cursiv_add_benchmark(webidl
      SOURCES webidl/bench/main.cpp)
endif()

cursiv_package_language(webidl)
