# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `microcGenerated` (the compiled library) and `microc_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID microc
    GRAMMAR    microc/core/MicrocGrammar.hpp
    NAMESPACE  microc::Microc
    EXTENSIONS .microc
    OUT        microc/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/microc/core/*.cpp)
add_library(microcCore ${_core})
target_link_libraries(microcCore PUBLIC microcGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/microc/cli/*.cpp)
add_executable(microc_cli ${_cli})
target_link_libraries(microc_cli
                      PRIVATE microcCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/microc/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/microc/lsp/main.cpp)
add_library(microcLsp ${_lsp})
target_link_libraries(microcLsp PUBLIC microcCore cursiv::lsp)
add_executable(microc_lsp microc/lsp/main.cpp)
target_link_libraries(microc_lsp PRIVATE microcLsp)

cursiv_add_benchmark(microc SOURCES microc/bench/main.cpp)

cursiv_package_language(microc)
