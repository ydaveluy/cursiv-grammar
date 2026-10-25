# Generate the parser and its language module.
cursiv_generate_language(ID javacc
    GRAMMAR    javacc/core/JavaccGrammar.hpp
    NAMESPACE  javacc::Javacc
    EXTENSIONS .jj .jjt
    OUT        javacc/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/javacc/core/*.cpp)
add_library(javaccCore ${_core})
target_link_libraries(javaccCore PUBLIC javaccGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/javacc/cli/*.cpp)
if(_cli)
  add_executable(javacc_cli ${_cli})
  target_link_libraries(javacc_cli
                        PRIVATE javaccCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/javacc/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/javacc/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/javacc/lsp/main.cpp)
  add_library(javaccLsp ${_lsp})
  target_link_libraries(javaccLsp PUBLIC javaccCore cursiv::lsp)
  add_executable(javacc_lsp javacc/lsp/main.cpp)
  target_link_libraries(javacc_lsp PRIVATE javaccLsp)
endif()

if(TARGET javaccLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/javacc/bench/main.cpp")
  cursiv_add_benchmark(javacc
      SOURCES javacc/bench/main.cpp)
endif()

cursiv_package_language(javacc)
