# Generate the parser and its language module.
cursiv_generate_language(ID alloy
    GRAMMAR    alloy/core/AlloyGrammar.hpp
    NAMESPACE  alloy::Alloy
    EXTENSIONS .als
    OUT        alloy/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/alloy/core/*.cpp)
add_library(alloyCore ${_core})
target_link_libraries(alloyCore PUBLIC alloyGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/alloy/cli/*.cpp)
if(_cli)
  add_executable(alloy_cli ${_cli})
  target_link_libraries(alloy_cli
                        PRIVATE alloyCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/alloy/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/alloy/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/alloy/lsp/main.cpp)
  add_library(alloyLsp ${_lsp})
  target_link_libraries(alloyLsp PUBLIC alloyCore cursiv::lsp)
  add_executable(alloy_lsp alloy/lsp/main.cpp)
  target_link_libraries(alloy_lsp PRIVATE alloyLsp)
endif()

if(TARGET alloyLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/alloy/bench/main.cpp")
  cursiv_add_benchmark(alloy
      SOURCES alloy/bench/main.cpp)
endif()

cursiv_package_language(alloy)
