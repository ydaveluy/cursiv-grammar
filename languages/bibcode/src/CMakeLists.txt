# Generate the parser and its language module.
cursiv_generate_language(ID bibcode
    GRAMMAR    bibcode/core/BibcodeGrammar.hpp
    NAMESPACE  bibcode::Bibcode
    EXTENSIONS .bibcode
    OUT        bibcode/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bibcode/core/*.cpp)
add_library(bibcodeCore ${_core})
target_link_libraries(bibcodeCore PUBLIC bibcodeGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bibcode/cli/*.cpp)
if(_cli)
  add_executable(bibcode_cli ${_cli})
  target_link_libraries(bibcode_cli
                        PRIVATE bibcodeCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bibcode/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/bibcode/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/bibcode/lsp/main.cpp)
  add_library(bibcodeLsp ${_lsp})
  target_link_libraries(bibcodeLsp PUBLIC bibcodeCore cursiv::lsp)
  add_executable(bibcode_lsp bibcode/lsp/main.cpp)
  target_link_libraries(bibcode_lsp PRIVATE bibcodeLsp)
endif()

if(TARGET bibcodeLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bibcode/bench/main.cpp")
  cursiv_add_benchmark(bibcode
      SOURCES bibcode/bench/main.cpp)
endif()

cursiv_package_language(bibcode)
