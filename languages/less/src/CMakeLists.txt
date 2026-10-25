# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `lessGenerated` (the compiled library) and `less_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID less
    GRAMMAR    less/core/LessGrammar.hpp
    NAMESPACE  less::Less
    EXTENSIONS .less
    OUT        less/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/less/core/*.cpp)
add_library(lessCore ${_core})
target_link_libraries(lessCore PUBLIC lessGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/less/cli/*.cpp)
add_executable(less_cli ${_cli})
target_link_libraries(less_cli
                      PRIVATE lessCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/less/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/less/lsp/main.cpp)
add_library(lessLsp ${_lsp})
target_link_libraries(lessLsp PUBLIC lessCore cursiv::lsp)
add_executable(less_lsp less/lsp/main.cpp)
target_link_libraries(less_lsp PRIVATE lessLsp)

cursiv_add_benchmark(less SOURCES less/bench/main.cpp)

cursiv_package_language(less)
