# Generate the parser and its language module.
cursiv_generate_language(ID pcre
    GRAMMAR    pcre/core/PcreGrammar.hpp
    NAMESPACE  pcre::Pcre
    EXTENSIONS .pcre
    OUT        pcre/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pcre/core/*.cpp)
add_library(pcreCore ${_core})
target_link_libraries(pcreCore PUBLIC pcreGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pcre/cli/*.cpp)
if(_cli)
  add_executable(pcre_cli ${_cli})
  target_link_libraries(pcre_cli
                        PRIVATE pcreCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/pcre/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/pcre/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/pcre/lsp/main.cpp)
  add_library(pcreLsp ${_lsp})
  target_link_libraries(pcreLsp PUBLIC pcreCore cursiv::lsp)
  add_executable(pcre_lsp pcre/lsp/main.cpp)
  target_link_libraries(pcre_lsp PRIVATE pcreLsp)
endif()

if(TARGET pcreLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/pcre/bench/main.cpp")
  cursiv_add_benchmark(pcre
      SOURCES pcre/bench/main.cpp)
endif()

cursiv_package_language(pcre)
