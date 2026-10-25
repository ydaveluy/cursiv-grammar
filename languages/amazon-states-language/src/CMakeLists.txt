# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `amazonstateslanguageGenerated` (the compiled library) and `amazonstateslanguage_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID amazonstateslanguage
    GRAMMAR    amazonstateslanguage/core/AmazonStatesLanguageGrammar.hpp
    NAMESPACE  amazonstateslanguage::AmazonStatesLanguage
    EXTENSIONS .asl
    OUT        amazonstateslanguage/generated)

# The field-name vocabulary (~120 keyword terminals sharing a handful of
# large alternations) needs more than the compiler's default template
# recursion depth.
target_compile_options(amazonstateslanguage_gen PRIVATE -ftemplate-depth=4000)
target_compile_options(amazonstateslanguageGenerated PRIVATE -ftemplate-depth=4000)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/amazonstateslanguage/core/*.cpp)
add_library(amazonstateslanguageCore ${_core})
target_link_libraries(amazonstateslanguageCore PUBLIC amazonstateslanguageGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/amazonstateslanguage/cli/*.cpp)
add_executable(amazonstateslanguage_cli ${_cli})
target_link_libraries(amazonstateslanguage_cli
                      PRIVATE amazonstateslanguageCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/amazonstateslanguage/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/amazonstateslanguage/lsp/main.cpp)
add_library(amazonstateslanguageLsp ${_lsp})
target_link_libraries(amazonstateslanguageLsp PUBLIC amazonstateslanguageCore cursiv::lsp)
add_executable(amazonstateslanguage_lsp amazonstateslanguage/lsp/main.cpp)
target_link_libraries(amazonstateslanguage_lsp PRIVATE amazonstateslanguageLsp)

cursiv_add_benchmark(amazonstateslanguage SOURCES amazonstateslanguage/bench/main.cpp)

cursiv_package_language(amazonstateslanguage)
