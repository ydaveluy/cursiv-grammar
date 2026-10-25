# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `oberonGenerated` (the compiled library) and `oberon_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID oberon
    GRAMMAR    oberon/core/OberonGrammar.hpp
    NAMESPACE  oberon::Oberon
    EXTENSIONS .mod
    OUT        oberon/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/oberon/core/*.cpp)
add_library(oberonCore ${_core})
target_link_libraries(oberonCore PUBLIC oberonGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/oberon/cli/*.cpp)
add_executable(oberon_cli ${_cli})
target_link_libraries(oberon_cli
                      PRIVATE oberonCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/oberon/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/oberon/lsp/main.cpp)
add_library(oberonLsp ${_lsp})
target_link_libraries(oberonLsp PUBLIC oberonCore cursiv::lsp)
add_executable(oberon_lsp oberon/lsp/main.cpp)
target_link_libraries(oberon_lsp PRIVATE oberonLsp)

cursiv_add_benchmark(oberon SOURCES oberon/bench/main.cpp)

cursiv_package_language(oberon)
