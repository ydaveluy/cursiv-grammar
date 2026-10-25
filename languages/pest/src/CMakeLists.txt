# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `pestGenerated` (the compiled library) and `pest_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID pest
    GRAMMAR    pest/core/PestGrammar.hpp
    NAMESPACE  pest::Pest
    EXTENSIONS .pest
    OUT        pest/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pest/core/*.cpp)
add_library(pestCore ${_core})
target_link_libraries(pestCore PUBLIC pestGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pest/cli/*.cpp)
add_executable(pest_cli ${_cli})
target_link_libraries(pest_cli
                      PRIVATE pestCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/pest/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/pest/lsp/main.cpp)
add_library(pestLsp ${_lsp})
target_link_libraries(pestLsp PUBLIC pestCore cursiv::lsp)
add_executable(pest_lsp pest/lsp/main.cpp)
target_link_libraries(pest_lsp PRIVATE pestLsp)

cursiv_add_benchmark(pest SOURCES pest/bench/main.cpp)

cursiv_package_language(pest)
