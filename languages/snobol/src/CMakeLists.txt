# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `snobolGenerated` (the compiled library) and `snobol_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID snobol
    GRAMMAR    snobol/core/SnobolGrammar.hpp
    NAMESPACE  snobol::Snobol
    EXTENSIONS .sno
    OUT        snobol/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/snobol/core/*.cpp)
add_library(snobolCore ${_core})
target_link_libraries(snobolCore PUBLIC snobolGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/snobol/cli/*.cpp)
add_executable(snobol_cli ${_cli})
target_link_libraries(snobol_cli
                      PRIVATE snobolCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/snobol/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/snobol/lsp/main.cpp)
add_library(snobolLsp ${_lsp})
target_link_libraries(snobolLsp PUBLIC snobolCore cursiv::lsp)
add_executable(snobol_lsp snobol/lsp/main.cpp)
target_link_libraries(snobol_lsp PRIVATE snobolLsp)

cursiv_add_benchmark(snobol SOURCES snobol/bench/main.cpp)

cursiv_package_language(snobol)
