# Generate the parser and its language module.
cursiv_generate_language(ID evmbytecode
    GRAMMAR    evmbytecode/core/EvmBytecodeGrammar.hpp
    NAMESPACE  evmbytecode::EvmBytecode
    EXTENSIONS .evm
    OUT        evmbytecode/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/evmbytecode/core/*.cpp)
add_library(evmbytecodeCore ${_core})
target_link_libraries(evmbytecodeCore PUBLIC evmbytecodeGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/evmbytecode/cli/*.cpp)
if(_cli)
  add_executable(evmbytecode_cli ${_cli})
  target_link_libraries(evmbytecode_cli
                        PRIVATE evmbytecodeCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/evmbytecode/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/evmbytecode/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/evmbytecode/lsp/main.cpp)
  add_library(evmbytecodeLsp ${_lsp})
  target_link_libraries(evmbytecodeLsp PUBLIC evmbytecodeCore cursiv::lsp)
  add_executable(evmbytecode_lsp evmbytecode/lsp/main.cpp)
  target_link_libraries(evmbytecode_lsp PRIVATE evmbytecodeLsp)
endif()

if(TARGET evmbytecodeLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/evmbytecode/bench/main.cpp")
  cursiv_add_benchmark(evmbytecode
      SOURCES evmbytecode/bench/main.cpp)
endif()

cursiv_package_language(evmbytecode)
