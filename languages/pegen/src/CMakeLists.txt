# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `pegenGenerated` (the compiled library) and `pegen_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID pegen
    GRAMMAR    pegen/core/PegenGrammar.hpp
    NAMESPACE  pegen::Pegen
    EXTENSIONS .gram
    OUT        pegen/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pegen/core/*.cpp)
add_library(pegenCore ${_core})
target_link_libraries(pegenCore PUBLIC pegenGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pegen/cli/*.cpp)
add_executable(pegen_cli ${_cli})
target_link_libraries(pegen_cli
                      PRIVATE pegenCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pegen/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/pegen/lsp/main.cpp)
add_library(pegenLsp ${_lsp})
target_link_libraries(pegenLsp PUBLIC pegenCore cursiv::lsp)
add_executable(pegen_lsp pegen/lsp/main.cpp)
target_link_libraries(pegen_lsp PRIVATE pegenLsp)

cursiv_add_benchmark(pegen SOURCES pegen/bench/main.cpp)

cursiv_package_language(pegen)
