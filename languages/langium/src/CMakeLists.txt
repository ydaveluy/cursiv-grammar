# Generate the parser and its language module.
cursiv_generate_language(ID langium
    GRAMMAR    langium/core/LangiumGrammar.hpp
    NAMESPACE  langium::Langium
    EXTENSIONS .langium
    OUT        langium/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/langium/core/*.cpp)
add_library(langiumCore ${_core})
target_link_libraries(langiumCore PUBLIC langiumGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/langium/cli/*.cpp)
if(_cli)
  add_executable(langium_cli ${_cli})
  target_link_libraries(langium_cli
                        PRIVATE langiumCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/langium/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/langium/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/langium/lsp/main.cpp)
  add_library(langiumLsp ${_lsp})
  target_link_libraries(langiumLsp PUBLIC langiumCore cursiv::lsp)
  add_executable(langium_lsp langium/lsp/main.cpp)
  target_link_libraries(langium_lsp PRIVATE langiumLsp)
endif()

if(TARGET langiumLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/langium/bench/main.cpp")
  cursiv_add_benchmark(langium
      SOURCES langium/bench/main.cpp)
endif()

cursiv_package_language(langium)
