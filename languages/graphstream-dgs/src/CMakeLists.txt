# Generate the parser and its language module.
cursiv_generate_language(ID graphstreamdgs
    GRAMMAR    graphstreamdgs/core/GraphstreamDgsGrammar.hpp
    NAMESPACE  graphstreamdgs::GraphstreamDgs
    EXTENSIONS .dgs
    OUT        graphstreamdgs/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/graphstreamdgs/core/*.cpp)
add_library(graphstreamdgsCore ${_core})
target_link_libraries(graphstreamdgsCore PUBLIC graphstreamdgsGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/graphstreamdgs/cli/*.cpp)
if(_cli)
  add_executable(graphstreamdgs_cli ${_cli})
  target_link_libraries(graphstreamdgs_cli
                        PRIVATE graphstreamdgsCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/graphstreamdgs/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/graphstreamdgs/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/graphstreamdgs/lsp/main.cpp)
  add_library(graphstreamdgsLsp ${_lsp})
  target_link_libraries(graphstreamdgsLsp PUBLIC graphstreamdgsCore cursiv::lsp)
  add_executable(graphstreamdgs_lsp graphstreamdgs/lsp/main.cpp)
  target_link_libraries(graphstreamdgs_lsp PRIVATE graphstreamdgsLsp)
endif()

if(TARGET graphstreamdgsLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/graphstreamdgs/bench/main.cpp")
  cursiv_add_benchmark(graphstreamdgs
      SOURCES graphstreamdgs/bench/main.cpp)
endif()

cursiv_package_language(graphstreamdgs)
