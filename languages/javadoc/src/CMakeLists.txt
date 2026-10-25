# Generate the parser and its language module.
cursiv_generate_language(ID javadoc
    GRAMMAR    javadoc/core/JavadocGrammar.hpp
    NAMESPACE  javadoc::Javadoc
    EXTENSIONS .javadoc
    OUT        javadoc/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/javadoc/core/*.cpp)
add_library(javadocCore ${_core})
target_link_libraries(javadocCore PUBLIC javadocGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/javadoc/cli/*.cpp)
if(_cli)
  add_executable(javadoc_cli ${_cli})
  target_link_libraries(javadoc_cli
                        PRIVATE javadocCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/javadoc/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/javadoc/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/javadoc/lsp/main.cpp)
  add_library(javadocLsp ${_lsp})
  target_link_libraries(javadocLsp PUBLIC javadocCore cursiv::lsp)
  add_executable(javadoc_lsp javadoc/lsp/main.cpp)
  target_link_libraries(javadoc_lsp PRIVATE javadocLsp)
endif()

if(TARGET javadocLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/javadoc/bench/main.cpp")
  cursiv_add_benchmark(javadoc
      SOURCES javadoc/bench/main.cpp)
endif()

cursiv_package_language(javadoc)
