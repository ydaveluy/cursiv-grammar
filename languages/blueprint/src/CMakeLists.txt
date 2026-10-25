# Generate the parser and its language module.
cursiv_generate_language(ID blueprint
    GRAMMAR    blueprint/core/BlueprintGrammar.hpp
    NAMESPACE  blueprint::Blueprint
    EXTENSIONS .bp
    OUT        blueprint/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/blueprint/core/*.cpp)
add_library(blueprintCore ${_core})
target_link_libraries(blueprintCore PUBLIC blueprintGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/blueprint/cli/*.cpp)
if(_cli)
  add_executable(blueprint_cli ${_cli})
  target_link_libraries(blueprint_cli
                        PRIVATE blueprintCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/blueprint/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/blueprint/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/blueprint/lsp/main.cpp)
  add_library(blueprintLsp ${_lsp})
  target_link_libraries(blueprintLsp PUBLIC blueprintCore cursiv::lsp)
  add_executable(blueprint_lsp blueprint/lsp/main.cpp)
  target_link_libraries(blueprint_lsp PRIVATE blueprintLsp)
endif()

if(TARGET blueprintLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/blueprint/bench/main.cpp")
  cursiv_add_benchmark(blueprint
      SOURCES blueprint/bench/main.cpp)
endif()

cursiv_package_language(blueprint)
