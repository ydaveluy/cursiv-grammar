# Generate the parser and its language module.
cursiv_generate_language(ID clf
    GRAMMAR    clf/core/ClfGrammar.hpp
    NAMESPACE  clf::Clf
    EXTENSIONS .clf
    OUT        clf/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/clf/core/*.cpp)
add_library(clfCore ${_core})
target_link_libraries(clfCore PUBLIC clfGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/clf/cli/*.cpp)
if(_cli)
  add_executable(clf_cli ${_cli})
  target_link_libraries(clf_cli
                        PRIVATE clfCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/clf/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/clf/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/clf/lsp/main.cpp)
  add_library(clfLsp ${_lsp})
  target_link_libraries(clfLsp PUBLIC clfCore cursiv::lsp)
  add_executable(clf_lsp clf/lsp/main.cpp)
  target_link_libraries(clf_lsp PRIVATE clfLsp)
endif()

if(TARGET clfLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/clf/bench/main.cpp")
  cursiv_add_benchmark(clf
      SOURCES clf/bench/main.cpp)
endif()

cursiv_package_language(clf)
