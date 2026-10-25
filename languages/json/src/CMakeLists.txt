# Generate the parser and its language module.
cursiv_generate_language(ID json
    GRAMMAR    json/core/JsonGrammar.hpp
    NAMESPACE  json::Json
    EXTENSIONS .json
    OUT        json/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/json/core/*.cpp)
add_library(jsonCore ${_core})
target_link_libraries(jsonCore PUBLIC jsonGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/json/cli/*.cpp)
if(_cli)
  add_executable(json_cli ${_cli})
  target_link_libraries(json_cli
                        PRIVATE jsonCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/json/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/json/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/json/lsp/main.cpp)
  add_library(jsonLsp ${_lsp})
  target_link_libraries(jsonLsp PUBLIC jsonCore cursiv::lsp)
  add_executable(json_lsp json/lsp/main.cpp)
  target_link_libraries(json_lsp PRIVATE jsonLsp)
endif()

if(TARGET jsonLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/json/bench/main.cpp")
  cursiv_add_benchmark(json
      SOURCES json/bench/main.cpp)
endif()

cursiv_package_language(json)
