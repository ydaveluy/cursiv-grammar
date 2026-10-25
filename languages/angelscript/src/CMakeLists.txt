# Generate the parser and its language module.
cursiv_generate_language(ID angelscript
    GRAMMAR    angelscript/core/AngelscriptGrammar.hpp
    NAMESPACE  angelscript::Angelscript
    EXTENSIONS .as
    OUT        angelscript/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/angelscript/core/*.cpp)
add_library(angelscriptCore ${_core})
target_link_libraries(angelscriptCore PUBLIC angelscriptGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/angelscript/cli/*.cpp)
if(_cli)
  add_executable(angelscript_cli ${_cli})
  target_link_libraries(angelscript_cli
                        PRIVATE angelscriptCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/angelscript/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/angelscript/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/angelscript/lsp/main.cpp)
  add_library(angelscriptLsp ${_lsp})
  target_link_libraries(angelscriptLsp PUBLIC angelscriptCore cursiv::lsp)
  add_executable(angelscript_lsp angelscript/lsp/main.cpp)
  target_link_libraries(angelscript_lsp PRIVATE angelscriptLsp)
endif()

if(TARGET angelscriptLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/angelscript/bench/main.cpp")
  cursiv_add_benchmark(angelscript
      SOURCES angelscript/bench/main.cpp)
endif()

cursiv_package_language(angelscript)
