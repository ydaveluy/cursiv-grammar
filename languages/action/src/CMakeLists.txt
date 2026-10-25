# Generate the parser and its language module.
cursiv_generate_language(ID action
    GRAMMAR    action/core/ActionGrammar.hpp
    NAMESPACE  action::Action
    EXTENSIONS .act
    OUT        action/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/action/core/*.cpp)
add_library(actionCore ${_core})
target_link_libraries(actionCore PUBLIC actionGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/action/cli/*.cpp)
if(_cli)
  add_executable(action_cli ${_cli})
  target_link_libraries(action_cli
                        PRIVATE actionCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/action/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/action/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/action/lsp/main.cpp)
  add_library(actionLsp ${_lsp})
  target_link_libraries(actionLsp PUBLIC actionCore cursiv::lsp)
  add_executable(action_lsp action/lsp/main.cpp)
  target_link_libraries(action_lsp PRIVATE actionLsp)
endif()

if(TARGET actionLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/action/bench/main.cpp")
  cursiv_add_benchmark(action
      SOURCES action/bench/main.cpp)
endif()

cursiv_package_language(action)
