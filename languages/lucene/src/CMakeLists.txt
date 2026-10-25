# Generate the parser and its language module.
cursiv_generate_language(ID lucene
    GRAMMAR    lucene/core/LuceneGrammar.hpp
    NAMESPACE  lucene::Lucene
    EXTENSIONS .lucene
    OUT        lucene/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lucene/core/*.cpp)
add_library(luceneCore ${_core})
target_link_libraries(luceneCore PUBLIC luceneGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lucene/cli/*.cpp)
if(_cli)
  add_executable(lucene_cli ${_cli})
  target_link_libraries(lucene_cli
                        PRIVATE luceneCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lucene/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/lucene/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/lucene/lsp/main.cpp)
  add_library(luceneLsp ${_lsp})
  target_link_libraries(luceneLsp PUBLIC luceneCore cursiv::lsp)
  add_executable(lucene_lsp lucene/lsp/main.cpp)
  target_link_libraries(lucene_lsp PRIVATE luceneLsp)
endif()

if(TARGET luceneLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/lucene/bench/main.cpp")
  cursiv_add_benchmark(lucene
      SOURCES lucene/bench/main.cpp)
endif()

cursiv_package_language(lucene)
