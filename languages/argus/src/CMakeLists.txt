# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `argusGenerated` (the compiled library) and `argus_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID argus
    GRAMMAR    argus/core/ArgusGrammar.hpp
    NAMESPACE  argus::Argus
    EXTENSIONS .arg
    OUT        argus/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/argus/core/*.cpp)
add_library(argusCore ${_core})
target_link_libraries(argusCore PUBLIC argusGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/argus/cli/*.cpp)
add_executable(argus_cli ${_cli})
target_link_libraries(argus_cli
                      PRIVATE argusCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/argus/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/argus/lsp/main.cpp)
add_library(argusLsp ${_lsp})
target_link_libraries(argusLsp PUBLIC argusCore cursiv::lsp)
add_executable(argus_lsp argus/lsp/main.cpp)
target_link_libraries(argus_lsp PRIVATE argusLsp)

cursiv_add_benchmark(argus SOURCES argus/bench/main.cpp)

cursiv_package_language(argus)
