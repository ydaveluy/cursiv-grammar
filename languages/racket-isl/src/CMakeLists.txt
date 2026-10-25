# Generate the parser and its language module.
cursiv_generate_language(ID racketisl
    GRAMMAR    racketisl/core/RacketIslGrammar.hpp
    NAMESPACE  racketisl::RacketIsl
    EXTENSIONS .racketisl .rkt
    OUT        racketisl/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/racketisl/core/*.cpp)
add_library(racketislCore ${_core})
target_link_libraries(racketislCore PUBLIC racketislGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/racketisl/cli/*.cpp)
if(_cli)
  add_executable(racketisl_cli ${_cli})
  target_link_libraries(racketisl_cli
                        PRIVATE racketislCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/racketisl/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/racketisl/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/racketisl/lsp/main.cpp)
  add_library(racketislLsp ${_lsp})
  target_link_libraries(racketislLsp PUBLIC racketislCore cursiv::lsp)
  add_executable(racketisl_lsp racketisl/lsp/main.cpp)
  target_link_libraries(racketisl_lsp PRIVATE racketislLsp)
endif()

if(TARGET racketislLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/racketisl/bench/main.cpp")
  cursiv_add_benchmark(racketisl
      SOURCES racketisl/bench/main.cpp)
endif()

cursiv_package_language(racketisl)
