# Generate the parser and its language module.
cursiv_generate_language(ID databank
    GRAMMAR    databank/core/DatabankGrammar.hpp
    NAMESPACE  databank::Databank
    EXTENSIONS .databank
    OUT        databank/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/databank/core/*.cpp)
add_library(databankCore ${_core})
target_link_libraries(databankCore PUBLIC databankGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/databank/cli/*.cpp)
if(_cli)
  add_executable(databank_cli ${_cli})
  target_link_libraries(databank_cli
                        PRIVATE databankCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/databank/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/databank/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/databank/lsp/main.cpp)
  add_library(databankLsp ${_lsp})
  target_link_libraries(databankLsp PUBLIC databankCore cursiv::lsp)
  add_executable(databank_lsp databank/lsp/main.cpp)
  target_link_libraries(databank_lsp PRIVATE databankLsp)
endif()

if(TARGET databankLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/databank/bench/main.cpp")
  cursiv_add_benchmark(databank
      SOURCES databank/bench/main.cpp)
endif()

cursiv_package_language(databank)
