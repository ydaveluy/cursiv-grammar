# Generate the parser and its language module.
cursiv_generate_language(ID microc
    GRAMMAR    microc/core/MicrocGrammar.hpp
    NAMESPACE  microc::Microc
    EXTENSIONS .microc
    OUT        microc/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/microc/core/*.cpp)
add_library(microcCore ${_core})
target_link_libraries(microcCore PUBLIC microcGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/microc/cli/*.cpp)
if(_cli)
  add_executable(microc_cli ${_cli})
  target_link_libraries(microc_cli
                        PRIVATE microcCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/microc/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/microc/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/microc/lsp/main.cpp)
  add_library(microcLsp ${_lsp})
  target_link_libraries(microcLsp PUBLIC microcCore cursiv::lsp)
  add_executable(microc_lsp microc/lsp/main.cpp)
  target_link_libraries(microc_lsp PRIVATE microcLsp)
endif()

if(TARGET microcLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/microc/bench/main.cpp")
  cursiv_add_benchmark(microc
      SOURCES microc/bench/main.cpp)
endif()

cursiv_package_language(microc)
