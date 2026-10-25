# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `spassGenerated` (the compiled library) and `spass_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID spass
    GRAMMAR    spass/core/SpassGrammar.hpp
    NAMESPACE  spass::Spass
    EXTENSIONS .spass
    OUT        spass/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/spass/core/*.cpp)
add_library(spassCore ${_core})
target_link_libraries(spassCore PUBLIC spassGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/spass/cli/*.cpp)
add_executable(spass_cli ${_cli})
target_link_libraries(spass_cli
                      PRIVATE spassCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/spass/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/spass/lsp/main.cpp)
add_library(spassLsp ${_lsp})
target_link_libraries(spassLsp PUBLIC spassCore cursiv::lsp)
add_executable(spass_lsp spass/lsp/main.cpp)
target_link_libraries(spass_lsp PRIVATE spassLsp)

cursiv_add_benchmark(spass SOURCES spass/bench/main.cpp)

cursiv_package_language(spass)
