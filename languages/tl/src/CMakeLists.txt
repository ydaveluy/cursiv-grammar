# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `tlGenerated` (the compiled library) and `tl_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID tl
    GRAMMAR    tl/core/TlGrammar.hpp
    NAMESPACE  tl::Tl
    EXTENSIONS .tl
    OUT        tl/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tl/core/*.cpp)
add_library(tlCore ${_core})
target_link_libraries(tlCore PUBLIC tlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tl/cli/*.cpp)
add_executable(tl_cli ${_cli})
target_link_libraries(tl_cli
                      PRIVATE tlCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tl/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/tl/lsp/main.cpp)
add_library(tlLsp ${_lsp})
target_link_libraries(tlLsp PUBLIC tlCore cursiv::lsp)
add_executable(tl_lsp tl/lsp/main.cpp)
target_link_libraries(tl_lsp PRIVATE tlLsp)

cursiv_add_benchmark(tl SOURCES tl/bench/main.cpp)

cursiv_package_language(tl)
