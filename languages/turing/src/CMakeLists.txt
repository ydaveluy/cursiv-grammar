# Generate the parser and its language module.
cursiv_generate_language(ID turing
    GRAMMAR    turing/core/TuringGrammar.hpp
    NAMESPACE  turing::Turing
    EXTENSIONS .t
    OUT        turing/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/turing/core/*.cpp)
add_library(turingCore ${_core})
target_link_libraries(turingCore PUBLIC turingGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/turing/cli/*.cpp)
if(_cli)
  add_executable(turing_cli ${_cli})
  target_link_libraries(turing_cli
                        PRIVATE turingCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/turing/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/turing/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/turing/lsp/main.cpp)
  add_library(turingLsp ${_lsp})
  target_link_libraries(turingLsp PUBLIC turingCore cursiv::lsp)
  add_executable(turing_lsp turing/lsp/main.cpp)
  target_link_libraries(turing_lsp PRIVATE turingLsp)
endif()

if(TARGET turingLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/turing/bench/main.cpp")
  cursiv_add_benchmark(turing
      SOURCES turing/bench/main.cpp)
endif()

cursiv_package_language(turing)
