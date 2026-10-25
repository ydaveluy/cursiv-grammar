# Generate the parser and its language module.
cursiv_generate_language(ID infosapient
    GRAMMAR    infosapient/core/InfosapientGrammar.hpp
    NAMESPACE  infosapient::Infosapient
    EXTENSIONS .infosapient
    OUT        infosapient/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/infosapient/core/*.cpp)
add_library(infosapientCore ${_core})
target_link_libraries(infosapientCore PUBLIC infosapientGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/infosapient/cli/*.cpp)
if(_cli)
  add_executable(infosapient_cli ${_cli})
  target_link_libraries(infosapient_cli
                        PRIVATE infosapientCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/infosapient/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/infosapient/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/infosapient/lsp/main.cpp)
  add_library(infosapientLsp ${_lsp})
  target_link_libraries(infosapientLsp PUBLIC infosapientCore cursiv::lsp)
  add_executable(infosapient_lsp infosapient/lsp/main.cpp)
  target_link_libraries(infosapient_lsp PRIVATE infosapientLsp)
endif()

if(TARGET infosapientLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/infosapient/bench/main.cpp")
  cursiv_add_benchmark(infosapient
      SOURCES infosapient/bench/main.cpp)
endif()

cursiv_package_language(infosapient)
