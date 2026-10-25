# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `lccGenerated` (the compiled library) and `lcc_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID lcc
    GRAMMAR    lcc/core/LccGrammar.hpp
    NAMESPACE  lcc::Lcc
    EXTENSIONS .lcc
    OUT        lcc/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lcc/core/*.cpp)
add_library(lccCore ${_core})
target_link_libraries(lccCore PUBLIC lccGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lcc/cli/*.cpp)
add_executable(lcc_cli ${_cli})
target_link_libraries(lcc_cli
                      PRIVATE lccCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lcc/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/lcc/lsp/main.cpp)
add_library(lccLsp ${_lsp})
target_link_libraries(lccLsp PUBLIC lccCore cursiv::lsp)
add_executable(lcc_lsp lcc/lsp/main.cpp)
target_link_libraries(lcc_lsp PRIVATE lccLsp)

cursiv_add_benchmark(lcc SOURCES lcc/bench/main.cpp)

cursiv_package_language(lcc)
