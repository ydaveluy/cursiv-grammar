# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `bcplGenerated` (the compiled library) and `bcpl_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID bcpl
    GRAMMAR    bcpl/core/BcplGrammar.hpp
    NAMESPACE  bcpl::Bcpl
    EXTENSIONS .bcpl
    OUT        bcpl/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bcpl/core/*.cpp)
add_library(bcplCore ${_core})
target_link_libraries(bcplCore PUBLIC bcplGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bcpl/cli/*.cpp)
add_executable(bcpl_cli ${_cli})
target_link_libraries(bcpl_cli
                      PRIVATE bcplCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/bcpl/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/bcpl/lsp/main.cpp)
add_library(bcplLsp ${_lsp})
target_link_libraries(bcplLsp PUBLIC bcplCore cursiv::lsp)
add_executable(bcpl_lsp bcpl/lsp/main.cpp)
target_link_libraries(bcpl_lsp PRIVATE bcplLsp)

cursiv_add_benchmark(bcpl SOURCES bcpl/bench/main.cpp)

cursiv_package_language(bcpl)
