# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `adaGenerated` (the compiled library) and `ada_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID ada
    GRAMMAR    ada/core/AdaGrammar.hpp
    NAMESPACE  ada::Ada
    EXTENSIONS .ada
    OUT        ada/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ada/core/*.cpp)
add_library(adaCore ${_core})
target_link_libraries(adaCore PUBLIC adaGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ada/cli/*.cpp)
add_executable(ada_cli ${_cli})
target_link_libraries(ada_cli
                      PRIVATE adaCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ada/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/ada/lsp/main.cpp)
add_library(adaLsp ${_lsp})
target_link_libraries(adaLsp PUBLIC adaCore cursiv::lsp)
add_executable(ada_lsp ada/lsp/main.cpp)
target_link_libraries(ada_lsp PRIVATE adaLsp)

cursiv_add_benchmark(ada SOURCES ada/bench/main.cpp)

cursiv_package_language(ada)
