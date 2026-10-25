# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `sharcGenerated` (the compiled library) and `sharc_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID sharc
    GRAMMAR    sharc/core/SharcGrammar.hpp
    NAMESPACE  sharc::Sharc
    EXTENSIONS .dsp
    OUT        sharc/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sharc/core/*.cpp)
add_library(sharcCore ${_core})
target_link_libraries(sharcCore PUBLIC sharcGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sharc/cli/*.cpp)
add_executable(sharc_cli ${_cli})
target_link_libraries(sharc_cli
                      PRIVATE sharcCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/sharc/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/sharc/lsp/main.cpp)
add_library(sharcLsp ${_lsp})
target_link_libraries(sharcLsp PUBLIC sharcCore cursiv::lsp)
add_executable(sharc_lsp sharc/lsp/main.cpp)
target_link_libraries(sharc_lsp PRIVATE sharcLsp)

cursiv_add_benchmark(sharc SOURCES sharc/bench/main.cpp)

cursiv_package_language(sharc)
