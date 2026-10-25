# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `logoGenerated` (the compiled library) and `logo_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID logo
    GRAMMAR    logo/core/LogoGrammar.hpp
    NAMESPACE  logo::Logo
    EXTENSIONS .logo
    OUT        logo/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/logo/core/*.cpp)
add_library(logoCore ${_core})
target_link_libraries(logoCore PUBLIC logoGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/logo/cli/*.cpp)
add_executable(logo_cli ${_cli})
target_link_libraries(logo_cli
                      PRIVATE logoCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/logo/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/logo/lsp/main.cpp)
add_library(logoLsp ${_lsp})
target_link_libraries(logoLsp PUBLIC logoCore cursiv::lsp)
add_executable(logo_lsp logo/lsp/main.cpp)
target_link_libraries(logo_lsp PRIVATE logoLsp)

cursiv_add_benchmark(logo SOURCES logo/bench/main.cpp)

cursiv_package_language(logo)
