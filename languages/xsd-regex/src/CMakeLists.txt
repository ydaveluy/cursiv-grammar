# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `xsdregexGenerated` (the compiled library) and `xsdregex_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID xsdregex
    GRAMMAR    xsdregex/core/XsdRegexGrammar.hpp
    NAMESPACE  xsdregex::XsdRegex
    EXTENSIONS .xsdregex
    OUT        xsdregex/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xsdregex/core/*.cpp)
add_library(xsdregexCore ${_core})
target_link_libraries(xsdregexCore PUBLIC xsdregexGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xsdregex/cli/*.cpp)
add_executable(xsdregex_cli ${_cli})
target_link_libraries(xsdregex_cli
                      PRIVATE xsdregexCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xsdregex/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/xsdregex/lsp/main.cpp)
add_library(xsdregexLsp ${_lsp})
target_link_libraries(xsdregexLsp PUBLIC xsdregexCore cursiv::lsp)
add_executable(xsdregex_lsp xsdregex/lsp/main.cpp)
target_link_libraries(xsdregex_lsp PRIVATE xsdregexLsp)

cursiv_add_benchmark(xsdregex SOURCES xsdregex/bench/main.cpp)

cursiv_package_language(xsdregex)
