# Generate the parser and its language module.
cursiv_generate_language(ID oberon
    GRAMMAR    oberon/core/OberonGrammar.hpp
    NAMESPACE  oberon::Oberon
    EXTENSIONS .mod
    OUT        oberon/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/oberon/core/*.cpp)
add_library(oberonCore ${_core})
target_link_libraries(oberonCore PUBLIC oberonGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/oberon/cli/*.cpp)
if(_cli)
  add_executable(oberon_cli ${_cli})
  target_link_libraries(oberon_cli
                        PRIVATE oberonCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/oberon/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/oberon/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/oberon/lsp/main.cpp)
  add_library(oberonLsp ${_lsp})
  target_link_libraries(oberonLsp PUBLIC oberonCore cursiv::lsp)
  add_executable(oberon_lsp oberon/lsp/main.cpp)
  target_link_libraries(oberon_lsp PRIVATE oberonLsp)
endif()

if(TARGET oberonLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/oberon/bench/main.cpp")
  cursiv_add_benchmark(oberon
      SOURCES oberon/bench/main.cpp)
endif()

cursiv_package_language(oberon)
