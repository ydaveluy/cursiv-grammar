# Generate the parser and its language module.
cursiv_generate_language(ID stringtemplate
    GRAMMAR    stringtemplate/core/StringtemplateGrammar.hpp
    NAMESPACE  stringtemplate::Stringtemplate
    EXTENSIONS .st
    OUT        stringtemplate/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/stringtemplate/core/*.cpp)
add_library(stringtemplateCore ${_core})
target_link_libraries(stringtemplateCore PUBLIC stringtemplateGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/stringtemplate/cli/*.cpp)
if(_cli)
  add_executable(stringtemplate_cli ${_cli})
  target_link_libraries(stringtemplate_cli
                        PRIVATE stringtemplateCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/stringtemplate/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/stringtemplate/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/stringtemplate/lsp/main.cpp)
  add_library(stringtemplateLsp ${_lsp})
  target_link_libraries(stringtemplateLsp PUBLIC stringtemplateCore cursiv::lsp)
  add_executable(stringtemplate_lsp stringtemplate/lsp/main.cpp)
  target_link_libraries(stringtemplate_lsp PRIVATE stringtemplateLsp)
endif()

if(TARGET stringtemplateLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/stringtemplate/bench/main.cpp")
  cursiv_add_benchmark(stringtemplate
      SOURCES stringtemplate/bench/main.cpp)
endif()

cursiv_package_language(stringtemplate)
