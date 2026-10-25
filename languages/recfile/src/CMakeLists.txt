# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `recfileGenerated` (the compiled library) and `recfile_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID recfile
    GRAMMAR    recfile/core/RecfileGrammar.hpp
    NAMESPACE  recfile::Recfile
    EXTENSIONS .rec
    OUT        recfile/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/recfile/core/*.cpp)
add_library(recfileCore ${_core})
target_link_libraries(recfileCore PUBLIC recfileGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/recfile/cli/*.cpp)
add_executable(recfile_cli ${_cli})
target_link_libraries(recfile_cli
                      PRIVATE recfileCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/recfile/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/recfile/lsp/main.cpp)
add_library(recfileLsp ${_lsp})
target_link_libraries(recfileLsp PUBLIC recfileCore cursiv::lsp)
add_executable(recfile_lsp recfile/lsp/main.cpp)
target_link_libraries(recfile_lsp PRIVATE recfileLsp)

cursiv_add_benchmark(recfile SOURCES recfile/bench/main.cpp)

cursiv_package_language(recfile)
