# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `qifGenerated` (the compiled library) and `qif_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID qif
    GRAMMAR    qif/core/QifGrammar.hpp
    NAMESPACE  qif::Qif
    EXTENSIONS .qif
    OUT        qif/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/qif/core/*.cpp)
add_library(qifCore ${_core})
target_link_libraries(qifCore PUBLIC qifGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/qif/cli/*.cpp)
add_executable(qif_cli ${_cli})
target_link_libraries(qif_cli
                      PRIVATE qifCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/qif/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/qif/lsp/main.cpp)
add_library(qifLsp ${_lsp})
target_link_libraries(qifLsp PUBLIC qifCore cursiv::lsp)
add_executable(qif_lsp qif/lsp/main.cpp)
target_link_libraries(qif_lsp PRIVATE qifLsp)

cursiv_add_benchmark(qif SOURCES qif/bench/main.cpp)

cursiv_package_language(qif)
