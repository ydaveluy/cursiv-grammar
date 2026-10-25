# Generate the parser and its language module.
cursiv_generate_language(ID scala
    GRAMMAR    scala/core/ScalaGrammar.hpp
    NAMESPACE  scala::Scala
    EXTENSIONS .scala
    OUT        scala/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/scala/core/*.cpp)
add_library(scalaCore ${_core})
target_link_libraries(scalaCore PUBLIC scalaGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/scala/cli/*.cpp)
if(_cli)
  add_executable(scala_cli ${_cli})
  target_link_libraries(scala_cli
                        PRIVATE scalaCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/scala/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/scala/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/scala/lsp/main.cpp)
  add_library(scalaLsp ${_lsp})
  target_link_libraries(scalaLsp PUBLIC scalaCore cursiv::lsp)
  add_executable(scala_lsp scala/lsp/main.cpp)
  target_link_libraries(scala_lsp PRIVATE scalaLsp)
endif()

if(TARGET scalaLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/scala/bench/main.cpp")
  cursiv_add_benchmark(scala
      SOURCES scala/bench/main.cpp)
endif()

cursiv_package_language(scala)
