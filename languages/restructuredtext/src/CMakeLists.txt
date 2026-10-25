# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `restructuredtextGenerated` (the compiled library) and `restructuredtext_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID restructuredtext
    GRAMMAR    restructuredtext/core/RestructuredtextGrammar.hpp
    NAMESPACE  restructuredtext::Restructuredtext
    EXTENSIONS .rst
    OUT        restructuredtext/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/restructuredtext/core/*.cpp)
add_library(restructuredtextCore ${_core})
target_link_libraries(restructuredtextCore PUBLIC restructuredtextGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/restructuredtext/cli/*.cpp)
add_executable(restructuredtext_cli ${_cli})
target_link_libraries(restructuredtext_cli
                      PRIVATE restructuredtextCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/restructuredtext/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/restructuredtext/lsp/main.cpp)
add_library(restructuredtextLsp ${_lsp})
target_link_libraries(restructuredtextLsp PUBLIC restructuredtextCore cursiv::lsp)
add_executable(restructuredtext_lsp restructuredtext/lsp/main.cpp)
target_link_libraries(restructuredtext_lsp PRIVATE restructuredtextLsp)

cursiv_add_benchmark(restructuredtext SOURCES restructuredtext/bench/main.cpp)

cursiv_package_language(restructuredtext)
