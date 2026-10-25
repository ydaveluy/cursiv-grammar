# Generate the parser and its language module.
cursiv_generate_language(ID solidity
    GRAMMAR    solidity/core/SolidityGrammar.hpp
    NAMESPACE  solidity::Solidity
    EXTENSIONS .sol
    OUT        solidity/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/solidity/core/*.cpp)
add_library(solidityCore ${_core})
target_link_libraries(solidityCore PUBLIC solidityGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/solidity/cli/*.cpp)
if(_cli)
  add_executable(solidity_cli ${_cli})
  target_link_libraries(solidity_cli
                        PRIVATE solidityCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/solidity/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/solidity/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/solidity/lsp/main.cpp)
  add_library(solidityLsp ${_lsp})
  target_link_libraries(solidityLsp PUBLIC solidityCore cursiv::lsp)
  add_executable(solidity_lsp solidity/lsp/main.cpp)
  target_link_libraries(solidity_lsp PRIVATE solidityLsp)
endif()

if(TARGET solidityLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/solidity/bench/main.cpp")
  cursiv_add_benchmark(solidity
      SOURCES solidity/bench/main.cpp)
endif()

cursiv_package_language(solidity)
