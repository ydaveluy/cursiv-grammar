# Generate the parser and its language module.
cursiv_generate_language(ID unrealangelscript
    GRAMMAR    unrealangelscript/core/UnrealAngelscriptGrammar.hpp
    NAMESPACE  unrealangelscript::UnrealAngelscript
    EXTENSIONS .as
    OUT        unrealangelscript/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/unrealangelscript/core/*.cpp)
add_library(unrealangelscriptCore ${_core})
target_link_libraries(unrealangelscriptCore PUBLIC unrealangelscriptGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/unrealangelscript/cli/*.cpp)
if(_cli)
  add_executable(unrealangelscript_cli ${_cli})
  target_link_libraries(unrealangelscript_cli
                        PRIVATE unrealangelscriptCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/unrealangelscript/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/unrealangelscript/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/unrealangelscript/lsp/main.cpp)
  add_library(unrealangelscriptLsp ${_lsp})
  target_link_libraries(unrealangelscriptLsp PUBLIC unrealangelscriptCore cursiv::lsp)
  add_executable(unrealangelscript_lsp unrealangelscript/lsp/main.cpp)
  target_link_libraries(unrealangelscript_lsp PRIVATE unrealangelscriptLsp)
endif()

if(TARGET unrealangelscriptLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/unrealangelscript/bench/main.cpp")
  cursiv_add_benchmark(unrealangelscript
      SOURCES unrealangelscript/bench/main.cpp)
endif()

cursiv_package_language(unrealangelscript)
