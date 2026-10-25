# Generate the parser and its language module.
cursiv_generate_language(ID ltl
    GRAMMAR    ltl/core/LtlGrammar.hpp
    NAMESPACE  ltl::Ltl
    EXTENSIONS .ltl
    OUT        ltl/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ltl/core/*.cpp)
add_library(ltlCore ${_core})
target_link_libraries(ltlCore PUBLIC ltlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ltl/cli/*.cpp)
if(_cli)
  add_executable(ltl_cli ${_cli})
  target_link_libraries(ltl_cli
                        PRIVATE ltlCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ltl/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/ltl/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/ltl/lsp/main.cpp)
  add_library(ltlLsp ${_lsp})
  target_link_libraries(ltlLsp PUBLIC ltlCore cursiv::lsp)
  add_executable(ltl_lsp ltl/lsp/main.cpp)
  target_link_libraries(ltl_lsp PRIVATE ltlLsp)
endif()

if(TARGET ltlLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ltl/bench/main.cpp")
  cursiv_add_benchmark(ltl
      SOURCES ltl/bench/main.cpp)
endif()

cursiv_package_language(ltl)
