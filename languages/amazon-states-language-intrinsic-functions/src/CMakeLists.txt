# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `amazonstateslanguageintrinsicfunctionsGenerated` (the compiled library) and `amazonstateslanguageintrinsicfunctions_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID amazonstateslanguageintrinsicfunctions
    GRAMMAR    amazonstateslanguageintrinsicfunctions/core/AmazonStatesLanguageIntrinsicFunctionsGrammar.hpp
    NAMESPACE  amazonstateslanguageintrinsicfunctions::AmazonStatesLanguageIntrinsicFunctions
    EXTENSIONS .asl-intrinsic
    OUT        amazonstateslanguageintrinsicfunctions/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/amazonstateslanguageintrinsicfunctions/core/*.cpp)
add_library(amazonstateslanguageintrinsicfunctionsCore ${_core})
target_link_libraries(amazonstateslanguageintrinsicfunctionsCore PUBLIC amazonstateslanguageintrinsicfunctionsGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/amazonstateslanguageintrinsicfunctions/cli/*.cpp)
add_executable(amazonstateslanguageintrinsicfunctions_cli ${_cli})
target_link_libraries(amazonstateslanguageintrinsicfunctions_cli
                      PRIVATE amazonstateslanguageintrinsicfunctionsCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/amazonstateslanguageintrinsicfunctions/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/amazonstateslanguageintrinsicfunctions/lsp/main.cpp)
add_library(amazonstateslanguageintrinsicfunctionsLsp ${_lsp})
target_link_libraries(amazonstateslanguageintrinsicfunctionsLsp PUBLIC amazonstateslanguageintrinsicfunctionsCore cursiv::lsp)
add_executable(amazonstateslanguageintrinsicfunctions_lsp amazonstateslanguageintrinsicfunctions/lsp/main.cpp)
target_link_libraries(amazonstateslanguageintrinsicfunctions_lsp PRIVATE amazonstateslanguageintrinsicfunctionsLsp)

cursiv_add_benchmark(amazonstateslanguageintrinsicfunctions SOURCES amazonstateslanguageintrinsicfunctions/bench/main.cpp)

cursiv_package_language(amazonstateslanguageintrinsicfunctions)
