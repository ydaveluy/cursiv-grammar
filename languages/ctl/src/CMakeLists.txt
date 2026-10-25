# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `ctlGenerated` (the compiled library) and `ctl_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID ctl
    GRAMMAR    ctl/core/CtlGrammar.hpp
    NAMESPACE  ctl::Ctl
    EXTENSIONS .ctl
    OUT        ctl/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ctl/core/*.cpp)
add_library(ctlCore ${_core})
target_link_libraries(ctlCore PUBLIC ctlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ctl/cli/*.cpp)
add_executable(ctl_cli ${_cli})
target_link_libraries(ctl_cli
                      PRIVATE ctlCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ctl/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/ctl/lsp/main.cpp)
add_library(ctlLsp ${_lsp})
target_link_libraries(ctlLsp PUBLIC ctlCore cursiv::lsp)
add_executable(ctl_lsp ctl/lsp/main.cpp)
target_link_libraries(ctl_lsp PRIVATE ctlLsp)

cursiv_add_benchmark(ctl SOURCES ctl/bench/main.cpp)

cursiv_package_language(ctl)
