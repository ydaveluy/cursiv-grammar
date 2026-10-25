# Generate the parser and its language module.
cursiv_generate_language(ID xsdregex
    GRAMMAR    xsdregex/core/XsdRegexGrammar.hpp
    NAMESPACE  xsdregex::XsdRegex
    EXTENSIONS .xsdregex
    OUT        xsdregex/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xsdregex/core/*.cpp)
add_library(xsdregexCore ${_core})
target_link_libraries(xsdregexCore PUBLIC xsdregexGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xsdregex/cli/*.cpp)
if(_cli)
  add_executable(xsdregex_cli ${_cli})
  target_link_libraries(xsdregex_cli
                        PRIVATE xsdregexCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/xsdregex/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/xsdregex/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/xsdregex/lsp/main.cpp)
  add_library(xsdregexLsp ${_lsp})
  target_link_libraries(xsdregexLsp PUBLIC xsdregexCore cursiv::lsp)
  add_executable(xsdregex_lsp xsdregex/lsp/main.cpp)
  target_link_libraries(xsdregex_lsp PRIVATE xsdregexLsp)
endif()

if(TARGET xsdregexLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/xsdregex/bench/main.cpp")
  cursiv_add_benchmark(xsdregex
      SOURCES xsdregex/bench/main.cpp)
endif()

cursiv_package_language(xsdregex)
