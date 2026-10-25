# Generate the parser and its language module.
cursiv_generate_language(ID turtledoc
    GRAMMAR    turtledoc/core/TurtleDocGrammar.hpp
    NAMESPACE  turtledoc::TurtleDoc
    EXTENSIONS .ttl
    OUT        turtledoc/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/turtledoc/core/*.cpp)
add_library(turtledocCore ${_core})
target_link_libraries(turtledocCore PUBLIC turtledocGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/turtledoc/cli/*.cpp)
if(_cli)
  add_executable(turtledoc_cli ${_cli})
  target_link_libraries(turtledoc_cli
                        PRIVATE turtledocCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/turtledoc/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/turtledoc/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/turtledoc/lsp/main.cpp)
  add_library(turtledocLsp ${_lsp})
  target_link_libraries(turtledocLsp PUBLIC turtledocCore cursiv::lsp)
  add_executable(turtledoc_lsp turtledoc/lsp/main.cpp)
  target_link_libraries(turtledoc_lsp PRIVATE turtledocLsp)
endif()

if(TARGET turtledocLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/turtledoc/bench/main.cpp")
  cursiv_add_benchmark(turtledoc
      SOURCES turtledoc/bench/main.cpp)
endif()

cursiv_package_language(turtledoc)
