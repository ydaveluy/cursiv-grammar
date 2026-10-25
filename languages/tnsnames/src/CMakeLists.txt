# Generate the parser and its language module.
cursiv_generate_language(ID tnsnames
    GRAMMAR    tnsnames/core/TnsnamesGrammar.hpp
    NAMESPACE  tnsnames::Tnsnames
    EXTENSIONS .ora
    OUT        tnsnames/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tnsnames/core/*.cpp)
add_library(tnsnamesCore ${_core})
target_link_libraries(tnsnamesCore PUBLIC tnsnamesGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tnsnames/cli/*.cpp)
if(_cli)
  add_executable(tnsnames_cli ${_cli})
  target_link_libraries(tnsnames_cli
                        PRIVATE tnsnamesCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tnsnames/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/tnsnames/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/tnsnames/lsp/main.cpp)
  add_library(tnsnamesLsp ${_lsp})
  target_link_libraries(tnsnamesLsp PUBLIC tnsnamesCore cursiv::lsp)
  add_executable(tnsnames_lsp tnsnames/lsp/main.cpp)
  target_link_libraries(tnsnames_lsp PRIVATE tnsnamesLsp)
endif()

if(TARGET tnsnamesLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tnsnames/bench/main.cpp")
  cursiv_add_benchmark(tnsnames
      SOURCES tnsnames/bench/main.cpp)
endif()

cursiv_package_language(tnsnames)
