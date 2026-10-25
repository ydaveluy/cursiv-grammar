# Generate the parser and its language module.
cursiv_generate_language(ID scss
    GRAMMAR    scss/core/ScssGrammar.hpp
    NAMESPACE  scss::Scss
    EXTENSIONS .scss
    OUT        scss/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/scss/core/*.cpp)
add_library(scssCore ${_core})
target_link_libraries(scssCore PUBLIC scssGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/scss/cli/*.cpp)
if(_cli)
  add_executable(scss_cli ${_cli})
  target_link_libraries(scss_cli
                        PRIVATE scssCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/scss/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/scss/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/scss/lsp/main.cpp)
  add_library(scssLsp ${_lsp})
  target_link_libraries(scssLsp PUBLIC scssCore cursiv::lsp)
  add_executable(scss_lsp scss/lsp/main.cpp)
  target_link_libraries(scss_lsp PRIVATE scssLsp)
endif()

if(TARGET scssLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/scss/bench/main.cpp")
  cursiv_add_benchmark(scss
      SOURCES scss/bench/main.cpp)
endif()

cursiv_package_language(scss)
