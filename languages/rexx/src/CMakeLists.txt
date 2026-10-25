# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `rexxGenerated` (the compiled library) and `rexx_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID rexx
    GRAMMAR    rexx/core/RexxGrammar.hpp
    NAMESPACE  rexx::Rexx
    EXTENSIONS .rex
    OUT        rexx/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rexx/core/*.cpp)
add_library(rexxCore ${_core})
target_link_libraries(rexxCore PUBLIC rexxGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rexx/cli/*.cpp)
add_executable(rexx_cli ${_cli})
target_link_libraries(rexx_cli
                      PRIVATE rexxCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rexx/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/rexx/lsp/main.cpp)
add_library(rexxLsp ${_lsp})
target_link_libraries(rexxLsp PUBLIC rexxCore cursiv::lsp)
add_executable(rexx_lsp rexx/lsp/main.cpp)
target_link_libraries(rexx_lsp PRIVATE rexxLsp)

cursiv_add_benchmark(rexx SOURCES rexx/bench/main.cpp)

cursiv_package_language(rexx)
