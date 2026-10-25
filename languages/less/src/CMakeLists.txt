# Generate the parser and its language module.
cursiv_generate_language(ID less
    GRAMMAR    less/core/LessGrammar.hpp
    NAMESPACE  less::Less
    EXTENSIONS .less
    OUT        less/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/less/core/*.cpp)
add_library(lessCore ${_core})
target_link_libraries(lessCore PUBLIC lessGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/less/cli/*.cpp)
if(_cli)
  add_executable(less_cli ${_cli})
  target_link_libraries(less_cli
                        PRIVATE lessCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/less/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/less/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/less/lsp/main.cpp)
  add_library(lessLsp ${_lsp})
  target_link_libraries(lessLsp PUBLIC lessCore cursiv::lsp)
  add_executable(less_lsp less/lsp/main.cpp)
  target_link_libraries(less_lsp PRIVATE lessLsp)
endif()

if(TARGET lessLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/less/bench/main.cpp")
  cursiv_add_benchmark(less
      SOURCES less/bench/main.cpp)
endif()

cursiv_package_language(less)
