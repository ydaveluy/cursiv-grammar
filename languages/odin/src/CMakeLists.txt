# Generate the parser and its language module.
cursiv_generate_language(ID odin
    GRAMMAR    odin/core/OdinGrammar.hpp
    NAMESPACE  odin::Odin
    EXTENSIONS .odin
    OUT        odin/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/odin/core/*.cpp)
add_library(odinCore ${_core})
target_link_libraries(odinCore PUBLIC odinGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/odin/cli/*.cpp)
if(_cli)
  add_executable(odin_cli ${_cli})
  target_link_libraries(odin_cli
                        PRIVATE odinCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/odin/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/odin/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/odin/lsp/main.cpp)
  add_library(odinLsp ${_lsp})
  target_link_libraries(odinLsp PUBLIC odinCore cursiv::lsp)
  add_executable(odin_lsp odin/lsp/main.cpp)
  target_link_libraries(odin_lsp PRIVATE odinLsp)
endif()

if(TARGET odinLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/odin/bench/main.cpp")
  cursiv_add_benchmark(odin
      SOURCES odin/bench/main.cpp)
endif()

cursiv_package_language(odin)
