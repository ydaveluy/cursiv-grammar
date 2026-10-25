# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `srtGenerated` (the compiled library) and `srt_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID srt
    GRAMMAR    srt/core/SrtGrammar.hpp
    NAMESPACE  srt::Srt
    EXTENSIONS .srt
    OUT        srt/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/srt/core/*.cpp)
add_library(srtCore ${_core})
target_link_libraries(srtCore PUBLIC srtGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/srt/cli/*.cpp)
add_executable(srt_cli ${_cli})
target_link_libraries(srt_cli
                      PRIVATE srtCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/srt/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/srt/lsp/main.cpp)
add_library(srtLsp ${_lsp})
target_link_libraries(srtLsp PUBLIC srtCore cursiv::lsp)
add_executable(srt_lsp srt/lsp/main.cpp)
target_link_libraries(srt_lsp PRIVATE srtLsp)

cursiv_add_benchmark(srt SOURCES srt/bench/main.cpp)

cursiv_package_language(srt)
