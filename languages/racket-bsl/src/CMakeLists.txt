# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `racketbslGenerated` (the compiled library) and `racketbsl_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID racketbsl
    GRAMMAR    racketbsl/core/RacketBslGrammar.hpp
    NAMESPACE  racketbsl::RacketBsl
    EXTENSIONS .rkt
    OUT        racketbsl/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/racketbsl/core/*.cpp)
add_library(racketbslCore ${_core})
target_link_libraries(racketbslCore PUBLIC racketbslGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/racketbsl/cli/*.cpp)
add_executable(racketbsl_cli ${_cli})
target_link_libraries(racketbsl_cli
                      PRIVATE racketbslCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/racketbsl/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/racketbsl/lsp/main.cpp)
add_library(racketbslLsp ${_lsp})
target_link_libraries(racketbslLsp PUBLIC racketbslCore cursiv::lsp)
add_executable(racketbsl_lsp racketbsl/lsp/main.cpp)
target_link_libraries(racketbsl_lsp PRIVATE racketbslLsp)

cursiv_add_benchmark(racketbsl SOURCES racketbsl/bench/main.cpp)

cursiv_package_language(racketbsl)
