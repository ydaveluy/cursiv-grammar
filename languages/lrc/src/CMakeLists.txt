# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `lrcGenerated` (the compiled library) and `lrc_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID lrc
    GRAMMAR    lrc/core/LrcGrammar.hpp
    NAMESPACE  lrc::Lrc
    EXTENSIONS .lrc
    OUT        lrc/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lrc/core/*.cpp)
add_library(lrcCore ${_core})
target_link_libraries(lrcCore PUBLIC lrcGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lrc/cli/*.cpp)
add_executable(lrc_cli ${_cli})
target_link_libraries(lrc_cli
                      PRIVATE lrcCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lrc/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/lrc/lsp/main.cpp)
add_library(lrcLsp ${_lsp})
target_link_libraries(lrcLsp PUBLIC lrcCore cursiv::lsp)
add_executable(lrc_lsp lrc/lsp/main.cpp)
target_link_libraries(lrc_lsp PRIVATE lrcLsp)

cursiv_add_benchmark(lrc SOURCES lrc/bench/main.cpp)

cursiv_package_language(lrc)
