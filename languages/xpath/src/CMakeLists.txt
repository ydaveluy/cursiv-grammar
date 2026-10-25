# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `xpathGenerated` (the compiled library) and `xpath_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID xpath
    GRAMMAR    xpath/core/XpathGrammar.hpp
    NAMESPACE  xpath::Xpath
    EXTENSIONS .xpath
    OUT        xpath/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xpath/core/*.cpp)
add_library(xpathCore ${_core})
target_link_libraries(xpathCore PUBLIC xpathGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xpath/cli/*.cpp)
add_executable(xpath_cli ${_cli})
target_link_libraries(xpath_cli
                      PRIVATE xpathCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/xpath/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/xpath/lsp/main.cpp)
add_library(xpathLsp ${_lsp})
target_link_libraries(xpathLsp PUBLIC xpathCore cursiv::lsp)
add_executable(xpath_lsp xpath/lsp/main.cpp)
target_link_libraries(xpath_lsp PRIVATE xpathLsp)

cursiv_add_benchmark(xpath SOURCES xpath/bench/main.cpp)

cursiv_package_language(xpath)
