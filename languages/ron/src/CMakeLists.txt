# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `ronGenerated` (the compiled library) and `ron_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID ron
    GRAMMAR    ron/core/RonGrammar.hpp
    NAMESPACE  ron::Ron
    EXTENSIONS .ron
    OUT        ron/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ron/core/*.cpp)
add_library(ronCore ${_core})
target_link_libraries(ronCore PUBLIC ronGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ron/cli/*.cpp)
add_executable(ron_cli ${_cli})
target_link_libraries(ron_cli
                      PRIVATE ronCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ron/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/ron/lsp/main.cpp)
add_library(ronLsp ${_lsp})
target_link_libraries(ronLsp PUBLIC ronCore cursiv::lsp)
add_executable(ron_lsp ron/lsp/main.cpp)
target_link_libraries(ron_lsp PRIVATE ronLsp)

cursiv_add_benchmark(ron SOURCES ron/bench/main.cpp)

cursiv_package_language(ron)
