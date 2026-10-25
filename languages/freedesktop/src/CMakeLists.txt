# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `desktopentryGenerated` (the compiled library) and `desktopentry_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID desktopentry
    GRAMMAR    desktopentry/core/DesktopEntryGrammar.hpp
    NAMESPACE  desktopentry::DesktopEntry
    EXTENSIONS .desktop
    OUT        desktopentry/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/desktopentry/core/*.cpp)
add_library(desktopentryCore ${_core})
target_link_libraries(desktopentryCore PUBLIC desktopentryGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/desktopentry/cli/*.cpp)
add_executable(desktopentry_cli ${_cli})
target_link_libraries(desktopentry_cli
                      PRIVATE desktopentryCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/desktopentry/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/desktopentry/lsp/main.cpp)
add_library(desktopentryLsp ${_lsp})
target_link_libraries(desktopentryLsp PUBLIC desktopentryCore cursiv::lsp)
add_executable(desktopentry_lsp desktopentry/lsp/main.cpp)
target_link_libraries(desktopentry_lsp PRIVATE desktopentryLsp)

cursiv_add_benchmark(desktopentry SOURCES desktopentry/bench/main.cpp)

cursiv_package_language(desktopentry)
