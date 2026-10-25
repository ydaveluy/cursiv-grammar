# Generate the parser and its language module.
cursiv_generate_language(ID robotwars
    GRAMMAR    robotwars/core/RobotwarsGrammar.hpp
    NAMESPACE  robotwars::Robotwars
    EXTENSIONS .robotwars
    OUT        robotwars/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/robotwars/core/*.cpp)
add_library(robotwarsCore ${_core})
target_link_libraries(robotwarsCore PUBLIC robotwarsGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/robotwars/cli/*.cpp)
if(_cli)
  add_executable(robotwars_cli ${_cli})
  target_link_libraries(robotwars_cli
                        PRIVATE robotwarsCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/robotwars/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/robotwars/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/robotwars/lsp/main.cpp)
  add_library(robotwarsLsp ${_lsp})
  target_link_libraries(robotwarsLsp PUBLIC robotwarsCore cursiv::lsp)
  add_executable(robotwars_lsp robotwars/lsp/main.cpp)
  target_link_libraries(robotwars_lsp PRIVATE robotwarsLsp)
endif()

if(TARGET robotwarsLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/robotwars/bench/main.cpp")
  cursiv_add_benchmark(robotwars
      SOURCES robotwars/bench/main.cpp)
endif()

cursiv_package_language(robotwars)
