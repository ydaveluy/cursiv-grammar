# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `lisaGenerated` (the compiled library) and `lisa_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID lisa
    GRAMMAR    lisa/core/LisaGrammar.hpp
    NAMESPACE  lisa::Lisa
    EXTENSIONS .lisa
    OUT        lisa/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lisa/core/*.cpp)
add_library(lisaCore ${_core})
target_link_libraries(lisaCore PUBLIC lisaGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lisa/cli/*.cpp)
add_executable(lisa_cli ${_cli})
target_link_libraries(lisa_cli
                      PRIVATE lisaCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lisa/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/lisa/lsp/main.cpp)
add_library(lisaLsp ${_lsp})
target_link_libraries(lisaLsp PUBLIC lisaCore cursiv::lsp)
add_executable(lisa_lsp lisa/lsp/main.cpp)
target_link_libraries(lisa_lsp PRIVATE lisaLsp)

cursiv_add_benchmark(lisa SOURCES lisa/bench/main.cpp)

cursiv_package_language(lisa)
