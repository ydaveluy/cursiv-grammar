# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `ardenGenerated` (the compiled library) and `arden_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID arden
    GRAMMAR    arden/core/ArdenGrammar.hpp
    NAMESPACE  arden::Arden
    EXTENSIONS .mlm
    OUT        arden/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/arden/core/*.cpp)
add_library(ardenCore ${_core})
target_link_libraries(ardenCore PUBLIC ardenGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/arden/cli/*.cpp)
add_executable(arden_cli ${_cli})
target_link_libraries(arden_cli
                      PRIVATE ardenCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/arden/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/arden/lsp/main.cpp)
add_library(ardenLsp ${_lsp})
target_link_libraries(ardenLsp PUBLIC ardenCore cursiv::lsp)
add_executable(arden_lsp arden/lsp/main.cpp)
target_link_libraries(arden_lsp PRIVATE ardenLsp)

cursiv_add_benchmark(arden SOURCES arden/bench/main.cpp)

cursiv_package_language(arden)
