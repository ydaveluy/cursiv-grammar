# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `metamathGenerated` (the compiled library) and `metamath_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID metamath
    GRAMMAR    metamath/core/MetamathGrammar.hpp
    NAMESPACE  metamath::Metamath
    EXTENSIONS .mm
    OUT        metamath/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/metamath/core/*.cpp)
add_library(metamathCore ${_core})
target_link_libraries(metamathCore PUBLIC metamathGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/metamath/cli/*.cpp)
add_executable(metamath_cli ${_cli})
target_link_libraries(metamath_cli
                      PRIVATE metamathCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/metamath/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/metamath/lsp/main.cpp)
add_library(metamathLsp ${_lsp})
target_link_libraries(metamathLsp PUBLIC metamathCore cursiv::lsp)
add_executable(metamath_lsp metamath/lsp/main.cpp)
target_link_libraries(metamath_lsp PRIVATE metamathLsp)

cursiv_add_benchmark(metamath SOURCES metamath/bench/main.cpp)

cursiv_package_language(metamath)
