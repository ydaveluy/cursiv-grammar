# Generate the parser and its language module.
cursiv_generate_language(ID racketbsl
    GRAMMAR    racketbsl/core/RacketBslGrammar.hpp
    NAMESPACE  racketbsl::RacketBsl
    EXTENSIONS .rkt
    OUT        racketbsl/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/racketbsl/core/*.cpp)
add_library(racketbslCore ${_core})
target_link_libraries(racketbslCore PUBLIC racketbslGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/racketbsl/cli/*.cpp)
if(_cli)
  add_executable(racketbsl_cli ${_cli})
  target_link_libraries(racketbsl_cli
                        PRIVATE racketbslCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/racketbsl/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/racketbsl/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/racketbsl/lsp/main.cpp)
  add_library(racketbslLsp ${_lsp})
  target_link_libraries(racketbslLsp PUBLIC racketbslCore cursiv::lsp)
  add_executable(racketbsl_lsp racketbsl/lsp/main.cpp)
  target_link_libraries(racketbsl_lsp PRIVATE racketbslLsp)
endif()

if(TARGET racketbslLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/racketbsl/bench/main.cpp")
  cursiv_add_benchmark(racketbsl
      SOURCES racketbsl/bench/main.cpp)
endif()

cursiv_package_language(racketbsl)
