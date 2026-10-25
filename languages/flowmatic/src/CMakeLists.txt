# Generate the parser and its language module.
cursiv_generate_language(ID flowmatic
    GRAMMAR    flowmatic/core/FlowmaticGrammar.hpp
    NAMESPACE  flowmatic::Flowmatic
    EXTENSIONS .flowmatic
    OUT        flowmatic/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/flowmatic/core/*.cpp)
add_library(flowmaticCore ${_core})
target_link_libraries(flowmaticCore PUBLIC flowmaticGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/flowmatic/cli/*.cpp)
if(_cli)
  add_executable(flowmatic_cli ${_cli})
  target_link_libraries(flowmatic_cli
                        PRIVATE flowmaticCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/flowmatic/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/flowmatic/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/flowmatic/lsp/main.cpp)
  add_library(flowmaticLsp ${_lsp})
  target_link_libraries(flowmaticLsp PUBLIC flowmaticCore cursiv::lsp)
  add_executable(flowmatic_lsp flowmatic/lsp/main.cpp)
  target_link_libraries(flowmatic_lsp PRIVATE flowmaticLsp)
endif()

if(TARGET flowmaticLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/flowmatic/bench/main.cpp")
  cursiv_add_benchmark(flowmatic
      SOURCES flowmatic/bench/main.cpp)
endif()

cursiv_package_language(flowmatic)
