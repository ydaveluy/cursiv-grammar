# Generate the parser and its language module.
cursiv_generate_language(ID aterm
    GRAMMAR    aterm/core/AtermGrammar.hpp
    NAMESPACE  aterm::Aterm
    EXTENSIONS .aterm
    OUT        aterm/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/aterm/core/*.cpp)
add_library(atermCore ${_core})
target_link_libraries(atermCore PUBLIC atermGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/aterm/cli/*.cpp)
if(_cli)
  add_executable(aterm_cli ${_cli})
  target_link_libraries(aterm_cli
                        PRIVATE atermCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/aterm/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/aterm/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/aterm/lsp/main.cpp)
  add_library(atermLsp ${_lsp})
  target_link_libraries(atermLsp PUBLIC atermCore cursiv::lsp)
  add_executable(aterm_lsp aterm/lsp/main.cpp)
  target_link_libraries(aterm_lsp PRIVATE atermLsp)
endif()

if(TARGET atermLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/aterm/bench/main.cpp")
  cursiv_add_benchmark(aterm
      SOURCES aterm/bench/main.cpp)
endif()

cursiv_package_language(aterm)
