# Generate the parser and its language module.
cursiv_generate_language(ID turtle
    GRAMMAR    turtle/core/TurtleGrammar.hpp
    NAMESPACE  turtle::Turtle
    EXTENSIONS .ttl
    OUT        turtle/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/turtle/core/*.cpp)
add_library(turtleCore ${_core})
target_link_libraries(turtleCore PUBLIC turtleGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/turtle/cli/*.cpp)
if(_cli)
  add_executable(turtle_cli ${_cli})
  target_link_libraries(turtle_cli
                        PRIVATE turtleCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/turtle/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/turtle/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/turtle/lsp/main.cpp)
  add_library(turtleLsp ${_lsp})
  target_link_libraries(turtleLsp PUBLIC turtleCore cursiv::lsp)
  add_executable(turtle_lsp turtle/lsp/main.cpp)
  target_link_libraries(turtle_lsp PRIVATE turtleLsp)
endif()

if(TARGET turtleLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/turtle/bench/main.cpp")
  cursiv_add_benchmark(turtle
      SOURCES turtle/bench/main.cpp)
endif()

cursiv_package_language(turtle)
