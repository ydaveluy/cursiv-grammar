# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `bibcodeGenerated` (the compiled library) and `bibcode_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID bibcode
    GRAMMAR    bibcode/core/BibcodeGrammar.hpp
    NAMESPACE  bibcode::Bibcode
    EXTENSIONS .bibcode
    OUT        bibcode/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bibcode/core/*.cpp)
add_library(bibcodeCore ${_core})
target_link_libraries(bibcodeCore PUBLIC bibcodeGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bibcode/cli/*.cpp)
add_executable(bibcode_cli ${_cli})
target_link_libraries(bibcode_cli
                      PRIVATE bibcodeCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bibcode/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/bibcode/lsp/main.cpp)
add_library(bibcodeLsp ${_lsp})
target_link_libraries(bibcodeLsp PUBLIC bibcodeCore cursiv::lsp)
add_executable(bibcode_lsp bibcode/lsp/main.cpp)
target_link_libraries(bibcode_lsp PRIVATE bibcodeLsp)

cursiv_add_benchmark(bibcode SOURCES bibcode/bench/main.cpp)

cursiv_package_language(bibcode)
