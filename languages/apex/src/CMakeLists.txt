# Generate the parser and its language module.
cursiv_generate_language(ID apex
    GRAMMAR    apex/core/ApexGrammar.hpp
    NAMESPACE  apex::Apex
    EXTENSIONS .cls
    OUT        apex/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/apex/core/*.cpp)
add_library(apexCore ${_core})
target_link_libraries(apexCore PUBLIC apexGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/apex/cli/*.cpp)
if(_cli)
  add_executable(apex_cli ${_cli})
  target_link_libraries(apex_cli
                        PRIVATE apexCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/apex/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/apex/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/apex/lsp/main.cpp)
  add_library(apexLsp ${_lsp})
  target_link_libraries(apexLsp PUBLIC apexCore cursiv::lsp)
  add_executable(apex_lsp apex/lsp/main.cpp)
  target_link_libraries(apex_lsp PRIVATE apexLsp)
endif()

if(TARGET apexLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/apex/bench/main.cpp")
  cursiv_add_benchmark(apex
      SOURCES apex/bench/main.cpp)
endif()

cursiv_package_language(apex)
