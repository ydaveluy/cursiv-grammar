# Generate the parser and its language module.
cursiv_generate_language(ID prolog
    GRAMMAR    prolog/core/PrologGrammar.hpp
    NAMESPACE  prolog::Prolog
    EXTENSIONS .pl
    OUT        prolog/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/prolog/core/*.cpp)
add_library(prologCore ${_core})
target_link_libraries(prologCore PUBLIC prologGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/prolog/cli/*.cpp)
if(_cli)
  add_executable(prolog_cli ${_cli})
  target_link_libraries(prolog_cli
                        PRIVATE prologCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/prolog/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/prolog/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/prolog/lsp/main.cpp)
  add_library(prologLsp ${_lsp})
  target_link_libraries(prologLsp PUBLIC prologCore cursiv::lsp)
  add_executable(prolog_lsp prolog/lsp/main.cpp)
  target_link_libraries(prolog_lsp PRIVATE prologLsp)
endif()

if(TARGET prologLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/prolog/bench/main.cpp")
  cursiv_add_benchmark(prolog
      SOURCES prolog/bench/main.cpp)
endif()

cursiv_package_language(prolog)
