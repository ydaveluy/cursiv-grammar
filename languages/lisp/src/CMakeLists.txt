# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `lispGenerated` (the compiled library) and `lisp_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID lisp
    GRAMMAR    lisp/core/LispGrammar.hpp
    NAMESPACE  lisp::Lisp
    EXTENSIONS .lisp
    OUT        lisp/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lisp/core/*.cpp)
add_library(lispCore ${_core})
target_link_libraries(lispCore PUBLIC lispGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lisp/cli/*.cpp)
add_executable(lisp_cli ${_cli})
target_link_libraries(lisp_cli
                      PRIVATE lispCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lisp/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/lisp/lsp/main.cpp)
add_library(lispLsp ${_lsp})
target_link_libraries(lispLsp PUBLIC lispCore cursiv::lsp)
add_executable(lisp_lsp lisp/lsp/main.cpp)
target_link_libraries(lisp_lsp PRIVATE lispLsp)

cursiv_add_benchmark(lisp SOURCES lisp/bench/main.cpp)

cursiv_package_language(lisp)
