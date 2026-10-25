# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `solidityGenerated` (the compiled library) and `solidity_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID solidity
    GRAMMAR    solidity/core/SolidityGrammar.hpp
    NAMESPACE  solidity::Solidity
    EXTENSIONS .sol
    OUT        solidity/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/solidity/core/*.cpp)
add_library(solidityCore ${_core})
target_link_libraries(solidityCore PUBLIC solidityGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/solidity/cli/*.cpp)
add_executable(solidity_cli ${_cli})
target_link_libraries(solidity_cli
                      PRIVATE solidityCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/solidity/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/solidity/lsp/main.cpp)
add_library(solidityLsp ${_lsp})
target_link_libraries(solidityLsp PUBLIC solidityCore cursiv::lsp)
add_executable(solidity_lsp solidity/lsp/main.cpp)
target_link_libraries(solidity_lsp PRIVATE solidityLsp)

cursiv_add_benchmark(solidity SOURCES solidity/bench/main.cpp)

cursiv_package_language(solidity)
