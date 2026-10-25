# Generate the parser and its language module.
cursiv_generate_language(ID ron
    GRAMMAR    ron/core/RonGrammar.hpp
    NAMESPACE  ron::Ron
    EXTENSIONS .ron
    OUT        ron/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ron/core/*.cpp)
add_library(ronCore ${_core})
target_link_libraries(ronCore PUBLIC ronGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ron/cli/*.cpp)
if(_cli)
  add_executable(ron_cli ${_cli})
  target_link_libraries(ron_cli
                        PRIVATE ronCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ron/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/ron/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/ron/lsp/main.cpp)
  add_library(ronLsp ${_lsp})
  target_link_libraries(ronLsp PUBLIC ronCore cursiv::lsp)
  add_executable(ron_lsp ron/lsp/main.cpp)
  target_link_libraries(ron_lsp PRIVATE ronLsp)
endif()

if(TARGET ronLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ron/bench/main.cpp")
  cursiv_add_benchmark(ron
      SOURCES ron/bench/main.cpp)
endif()

cursiv_package_language(ron)
