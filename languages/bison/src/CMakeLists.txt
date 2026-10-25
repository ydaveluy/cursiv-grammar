# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `bisonGenerated` (the compiled library) and `bison_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID bison
    GRAMMAR    bison/core/BisonGrammar.hpp
    NAMESPACE  bison::Bison
    EXTENSIONS .y
    OUT        bison/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bison/core/*.cpp)
add_library(bisonCore ${_core})
target_link_libraries(bisonCore PUBLIC bisonGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bison/cli/*.cpp)
add_executable(bison_cli ${_cli})
target_link_libraries(bison_cli
                      PRIVATE bisonCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bison/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/bison/lsp/main.cpp)
add_library(bisonLsp ${_lsp})
target_link_libraries(bisonLsp PUBLIC bisonCore cursiv::lsp)
add_executable(bison_lsp bison/lsp/main.cpp)
target_link_libraries(bison_lsp PRIVATE bisonLsp)

cursiv_add_benchmark(bison SOURCES bison/bench/main.cpp)

cursiv_package_language(bison)
