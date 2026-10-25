# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `fastaGenerated` (the compiled library) and `fasta_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID fasta
    GRAMMAR    fasta/core/FastaGrammar.hpp
    NAMESPACE  fasta::Fasta
    EXTENSIONS .fasta
    OUT        fasta/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fasta/core/*.cpp)
add_library(fastaCore ${_core})
target_link_libraries(fastaCore PUBLIC fastaGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fasta/cli/*.cpp)
add_executable(fasta_cli ${_cli})
target_link_libraries(fasta_cli
                      PRIVATE fastaCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/fasta/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/fasta/lsp/main.cpp)
add_library(fastaLsp ${_lsp})
target_link_libraries(fastaLsp PUBLIC fastaCore cursiv::lsp)
add_executable(fasta_lsp fasta/lsp/main.cpp)
target_link_libraries(fasta_lsp PRIVATE fastaLsp)

cursiv_add_benchmark(fasta SOURCES fasta/bench/main.cpp)

cursiv_package_language(fasta)
