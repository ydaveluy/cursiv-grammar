# Generate the parser and its language module.
cursiv_generate_language(ID gdscript
    GRAMMAR    gdscript/core/GdscriptGrammar.hpp
    NAMESPACE  gdscript::Gdscript
    EXTENSIONS .gd
    OUT        gdscript/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gdscript/core/*.cpp)
add_library(gdscriptCore ${_core})
target_link_libraries(gdscriptCore PUBLIC gdscriptGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/gdscript/cli/*.cpp)
if(_cli)
  add_executable(gdscript_cli ${_cli})
  target_link_libraries(gdscript_cli
                        PRIVATE gdscriptCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/gdscript/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/gdscript/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/gdscript/lsp/main.cpp)
  add_library(gdscriptLsp ${_lsp})
  target_link_libraries(gdscriptLsp PUBLIC gdscriptCore cursiv::lsp)
  add_executable(gdscript_lsp gdscript/lsp/main.cpp)
  target_link_libraries(gdscript_lsp PRIVATE gdscriptLsp)
endif()

if(TARGET gdscriptLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gdscript/bench/main.cpp")
  cursiv_add_benchmark(gdscript
      SOURCES gdscript/bench/main.cpp)
endif()

cursiv_package_language(gdscript)
