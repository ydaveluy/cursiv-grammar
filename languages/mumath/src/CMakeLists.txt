# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `mumathGenerated` (the compiled library) and `mumath_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID mumath
    GRAMMAR    mumath/core/MumathGrammar.hpp
    NAMESPACE  mumath::Mumath
    EXTENSIONS .mu
    OUT        mumath/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/mumath/core/*.cpp)
add_library(mumathCore ${_core})
target_link_libraries(mumathCore PUBLIC mumathGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/mumath/cli/*.cpp)
add_executable(mumath_cli ${_cli})
target_link_libraries(mumath_cli
                      PRIVATE mumathCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/mumath/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/mumath/lsp/main.cpp)
add_library(mumathLsp ${_lsp})
target_link_libraries(mumathLsp PUBLIC mumathCore cursiv::lsp)
add_executable(mumath_lsp mumath/lsp/main.cpp)
target_link_libraries(mumath_lsp PRIVATE mumathLsp)

cursiv_add_benchmark(mumath SOURCES mumath/bench/main.cpp)

cursiv_package_language(mumath)
