# Generate the parser and its language module.
cursiv_generate_language(ID velocity
    GRAMMAR    velocity/core/VelocityGrammar.hpp
    NAMESPACE  velocity::Velocity
    EXTENSIONS .vm
    OUT        velocity/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/velocity/core/*.cpp)
add_library(velocityCore ${_core})
target_link_libraries(velocityCore PUBLIC velocityGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/velocity/cli/*.cpp)
if(_cli)
  add_executable(velocity_cli ${_cli})
  target_link_libraries(velocity_cli
                        PRIVATE velocityCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/velocity/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/velocity/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/velocity/lsp/main.cpp)
  add_library(velocityLsp ${_lsp})
  target_link_libraries(velocityLsp PUBLIC velocityCore cursiv::lsp)
  add_executable(velocity_lsp velocity/lsp/main.cpp)
  target_link_libraries(velocity_lsp PRIVATE velocityLsp)
endif()

if(TARGET velocityLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/velocity/bench/main.cpp")
  cursiv_add_benchmark(velocity
      SOURCES velocity/bench/main.cpp)
endif()

cursiv_package_language(velocity)
