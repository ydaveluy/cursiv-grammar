# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `acmeGenerated` (the compiled library) and `acme_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID acme
    GRAMMAR    acme/core/AcmeGrammar.hpp
    NAMESPACE  acme::Acme
    EXTENSIONS .acme
    OUT        acme/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/acme/core/*.cpp)
add_library(acmeCore ${_core})
target_link_libraries(acmeCore PUBLIC acmeGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/acme/cli/*.cpp)
add_executable(acme_cli ${_cli})
target_link_libraries(acme_cli
                      PRIVATE acmeCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/acme/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/acme/lsp/main.cpp)
add_library(acmeLsp ${_lsp})
target_link_libraries(acmeLsp PUBLIC acmeCore cursiv::lsp)
add_executable(acme_lsp acme/lsp/main.cpp)
target_link_libraries(acme_lsp PRIVATE acmeLsp)

cursiv_add_benchmark(acme SOURCES acme/bench/main.cpp)

cursiv_package_language(acme)
