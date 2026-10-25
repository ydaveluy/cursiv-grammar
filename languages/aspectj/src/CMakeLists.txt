# Generate the parser and its language module.
cursiv_generate_language(ID aspectj
    GRAMMAR    aspectj/core/AspectjGrammar.hpp
    NAMESPACE  aspectj::Aspectj
    EXTENSIONS .aj
    OUT        aspectj/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/aspectj/core/*.cpp)
add_library(aspectjCore ${_core})
target_link_libraries(aspectjCore PUBLIC aspectjGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/aspectj/cli/*.cpp)
if(_cli)
  add_executable(aspectj_cli ${_cli})
  target_link_libraries(aspectj_cli
                        PRIVATE aspectjCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/aspectj/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/aspectj/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/aspectj/lsp/main.cpp)
  add_library(aspectjLsp ${_lsp})
  target_link_libraries(aspectjLsp PUBLIC aspectjCore cursiv::lsp)
  add_executable(aspectj_lsp aspectj/lsp/main.cpp)
  target_link_libraries(aspectj_lsp PRIVATE aspectjLsp)
endif()

if(TARGET aspectjLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/aspectj/bench/main.cpp")
  cursiv_add_benchmark(aspectj
      SOURCES aspectj/bench/main.cpp)
endif()

cursiv_package_language(aspectj)
