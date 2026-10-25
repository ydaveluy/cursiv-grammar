# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `cayenneGenerated` (the compiled library) and `cayenne_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID cayenne
    GRAMMAR    cayenne/core/CayenneGrammar.hpp
    NAMESPACE  cayenne::Cayenne
    EXTENSIONS .cayenne
    OUT        cayenne/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cayenne/core/*.cpp)
add_library(cayenneCore ${_core})
target_link_libraries(cayenneCore PUBLIC cayenneGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cayenne/cli/*.cpp)
add_executable(cayenne_cli ${_cli})
target_link_libraries(cayenne_cli
                      PRIVATE cayenneCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cayenne/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/cayenne/lsp/main.cpp)
add_library(cayenneLsp ${_lsp})
target_link_libraries(cayenneLsp PUBLIC cayenneCore cursiv::lsp)
add_executable(cayenne_lsp cayenne/lsp/main.cpp)
target_link_libraries(cayenne_lsp PRIVATE cayenneLsp)

cursiv_add_benchmark(cayenne SOURCES cayenne/bench/main.cpp)

cursiv_package_language(cayenne)
