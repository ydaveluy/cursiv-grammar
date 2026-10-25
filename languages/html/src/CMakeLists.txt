# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `htmlGenerated` (the compiled library) and `html_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID html
    GRAMMAR    html/core/HtmlGrammar.hpp
    NAMESPACE  html::Html
    EXTENSIONS .html
    OUT        html/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/html/core/*.cpp)
add_library(htmlCore ${_core})
target_link_libraries(htmlCore PUBLIC htmlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/html/cli/*.cpp)
add_executable(html_cli ${_cli})
target_link_libraries(html_cli
                      PRIVATE htmlCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/html/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/html/lsp/main.cpp)
add_library(htmlLsp ${_lsp})
target_link_libraries(htmlLsp PUBLIC htmlCore cursiv::lsp)
add_executable(html_lsp html/lsp/main.cpp)
target_link_libraries(html_lsp PRIVATE htmlLsp)

cursiv_add_benchmark(html SOURCES html/bench/main.cpp)

cursiv_package_language(html)
