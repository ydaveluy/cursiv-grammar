# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `racketislGenerated` (the compiled library) and `racketisl_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID racketisl
    GRAMMAR    racketisl/core/RacketIslGrammar.hpp
    NAMESPACE  racketisl::RacketIsl
    EXTENSIONS .racketisl .rkt
    OUT        racketisl/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/racketisl/core/*.cpp)
add_library(racketislCore ${_core})
target_link_libraries(racketislCore PUBLIC racketislGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/racketisl/cli/*.cpp)
add_executable(racketisl_cli ${_cli})
target_link_libraries(racketisl_cli
                      PRIVATE racketislCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/racketisl/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/racketisl/lsp/main.cpp)
add_library(racketislLsp ${_lsp})
target_link_libraries(racketislLsp PUBLIC racketislCore cursiv::lsp)
add_executable(racketisl_lsp racketisl/lsp/main.cpp)
target_link_libraries(racketisl_lsp PRIVATE racketislLsp)

cursiv_add_benchmark(racketisl SOURCES racketisl/bench/main.cpp)

cursiv_package_language(racketisl)
