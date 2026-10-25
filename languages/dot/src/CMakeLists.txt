# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `dotGenerated` (the compiled library) and `dot_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID dot
    GRAMMAR    dot/core/DotGrammar.hpp
    NAMESPACE  dot::Dot
    EXTENSIONS .dot
    OUT        dot/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/dot/core/*.cpp)
add_library(dotCore ${_core})
target_link_libraries(dotCore PUBLIC dotGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/dot/cli/*.cpp)
add_executable(dot_cli ${_cli})
target_link_libraries(dot_cli
                      PRIVATE dotCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/dot/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/dot/lsp/main.cpp)
add_library(dotLsp ${_lsp})
target_link_libraries(dotLsp PUBLIC dotCore cursiv::lsp)
add_executable(dot_lsp dot/lsp/main.cpp)
target_link_libraries(dot_lsp PRIVATE dotLsp)

cursiv_add_benchmark(dot SOURCES dot/bench/main.cpp)

cursiv_package_language(dot)
