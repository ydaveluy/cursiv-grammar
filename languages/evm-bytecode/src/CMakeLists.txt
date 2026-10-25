# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `evmbytecodeGenerated` (the compiled library) and `evmbytecode_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID evmbytecode
    GRAMMAR    evmbytecode/core/EvmBytecodeGrammar.hpp
    NAMESPACE  evmbytecode::EvmBytecode
    EXTENSIONS .evm
    OUT        evmbytecode/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/evmbytecode/core/*.cpp)
add_library(evmbytecodeCore ${_core})
target_link_libraries(evmbytecodeCore PUBLIC evmbytecodeGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/evmbytecode/cli/*.cpp)
add_executable(evmbytecode_cli ${_cli})
target_link_libraries(evmbytecode_cli
                      PRIVATE evmbytecodeCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/evmbytecode/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/evmbytecode/lsp/main.cpp)
add_library(evmbytecodeLsp ${_lsp})
target_link_libraries(evmbytecodeLsp PUBLIC evmbytecodeCore cursiv::lsp)
add_executable(evmbytecode_lsp evmbytecode/lsp/main.cpp)
target_link_libraries(evmbytecode_lsp PRIVATE evmbytecodeLsp)

cursiv_add_benchmark(evmbytecode SOURCES evmbytecode/bench/main.cpp)

cursiv_package_language(evmbytecode)
