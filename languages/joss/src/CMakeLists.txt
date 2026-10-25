# Generate the parser and its language module.
cursiv_generate_language(ID joss
    GRAMMAR    joss/core/JossGrammar.hpp
    NAMESPACE  joss::Joss
    EXTENSIONS .joss
    OUT        joss/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/joss/core/*.cpp)
add_library(jossCore ${_core})
target_link_libraries(jossCore PUBLIC jossGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/joss/cli/*.cpp)
if(_cli)
  add_executable(joss_cli ${_cli})
  target_link_libraries(joss_cli
                        PRIVATE jossCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/joss/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/joss/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/joss/lsp/main.cpp)
  add_library(jossLsp ${_lsp})
  target_link_libraries(jossLsp PUBLIC jossCore cursiv::lsp)
  add_executable(joss_lsp joss/lsp/main.cpp)
  target_link_libraries(joss_lsp PRIVATE jossLsp)
endif()

if(TARGET jossLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/joss/bench/main.cpp")
  cursiv_add_benchmark(joss
      SOURCES joss/bench/main.cpp)
endif()

cursiv_package_language(joss)
