# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `morsecodeGenerated` (the compiled library) and `morsecode_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID morsecode
    GRAMMAR    morsecode/core/MorsecodeGrammar.hpp
    NAMESPACE  morsecode::Morsecode
    EXTENSIONS .morsecode
    OUT        morsecode/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/morsecode/core/*.cpp)
add_library(morsecodeCore ${_core})
target_link_libraries(morsecodeCore PUBLIC morsecodeGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/morsecode/cli/*.cpp)
add_executable(morsecode_cli ${_cli})
target_link_libraries(morsecode_cli
                      PRIVATE morsecodeCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/morsecode/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/morsecode/lsp/main.cpp)
add_library(morsecodeLsp ${_lsp})
target_link_libraries(morsecodeLsp PUBLIC morsecodeCore cursiv::lsp)
add_executable(morsecode_lsp morsecode/lsp/main.cpp)
target_link_libraries(morsecode_lsp PRIVATE morsecodeLsp)

cursiv_add_benchmark(morsecode SOURCES morsecode/bench/main.cpp)

cursiv_package_language(morsecode)
