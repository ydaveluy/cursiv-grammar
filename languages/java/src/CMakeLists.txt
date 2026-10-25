# Generate the parser and its language module.
cursiv_generate_language(ID java
    GRAMMAR    java/core/JavaGrammar.hpp
    NAMESPACE  java::Java
    EXTENSIONS .java
    OUT        java/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/java/core/*.cpp)
add_library(javaCore ${_core})
target_link_libraries(javaCore PUBLIC javaGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/java/cli/*.cpp)
if(_cli)
  add_executable(java_cli ${_cli})
  target_link_libraries(java_cli
                        PRIVATE javaCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/java/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/java/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/java/lsp/main.cpp)
  add_library(javaLsp ${_lsp})
  target_link_libraries(javaLsp PUBLIC javaCore cursiv::lsp)
  add_executable(java_lsp java/lsp/main.cpp)
  target_link_libraries(java_lsp PRIVATE javaLsp)
endif()

if(TARGET javaLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/java/bench/main.cpp")
  cursiv_add_benchmark(java
      SOURCES java/bench/main.cpp)
endif()

cursiv_package_language(java)
