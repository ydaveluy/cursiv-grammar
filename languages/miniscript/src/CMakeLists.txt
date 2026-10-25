# Generate the parser and its language module.
cursiv_generate_language(ID miniscript
    GRAMMAR    miniscript/core/MiniscriptGrammar.hpp
    NAMESPACE  miniscript::Miniscript
    EXTENSIONS .ms
    OUT        miniscript/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/miniscript/core/*.cpp)
add_library(miniscriptCore ${_core})
target_link_libraries(miniscriptCore PUBLIC miniscriptGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/miniscript/cli/*.cpp)
if(_cli)
  add_executable(miniscript_cli ${_cli})
  target_link_libraries(miniscript_cli
                        PRIVATE miniscriptCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/miniscript/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/miniscript/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/miniscript/lsp/main.cpp)
  add_library(miniscriptLsp ${_lsp})
  target_link_libraries(miniscriptLsp PUBLIC miniscriptCore cursiv::lsp)
  add_executable(miniscript_lsp miniscript/lsp/main.cpp)
  target_link_libraries(miniscript_lsp PRIVATE miniscriptLsp)
endif()

if(TARGET miniscriptLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/miniscript/bench/main.cpp")
  cursiv_add_benchmark(miniscript
      SOURCES miniscript/bench/main.cpp)
endif()

cursiv_package_language(miniscript)
