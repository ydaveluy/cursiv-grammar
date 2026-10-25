# Generate the parser and its language module.
cursiv_generate_language(ID sharc
    GRAMMAR    sharc/core/SharcGrammar.hpp
    NAMESPACE  sharc::Sharc
    EXTENSIONS .dsp
    OUT        sharc/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sharc/core/*.cpp)
add_library(sharcCore ${_core})
target_link_libraries(sharcCore PUBLIC sharcGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sharc/cli/*.cpp)
if(_cli)
  add_executable(sharc_cli ${_cli})
  target_link_libraries(sharc_cli
                        PRIVATE sharcCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/sharc/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/sharc/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/sharc/lsp/main.cpp)
  add_library(sharcLsp ${_lsp})
  target_link_libraries(sharcLsp PUBLIC sharcCore cursiv::lsp)
  add_executable(sharc_lsp sharc/lsp/main.cpp)
  target_link_libraries(sharc_lsp PRIVATE sharcLsp)
endif()

if(TARGET sharcLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/sharc/bench/main.cpp")
  cursiv_add_benchmark(sharc
      SOURCES sharc/bench/main.cpp)
endif()

cursiv_package_language(sharc)
