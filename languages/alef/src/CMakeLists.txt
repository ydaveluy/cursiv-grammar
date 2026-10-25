# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `alefGenerated` (the compiled library) and `alef_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID alef
    GRAMMAR    alef/core/AlefGrammar.hpp
    NAMESPACE  alef::Alef
    EXTENSIONS .alef
    OUT        alef/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/alef/core/*.cpp)
add_library(alefCore ${_core})
target_link_libraries(alefCore PUBLIC alefGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/alef/cli/*.cpp)
add_executable(alef_cli ${_cli})
target_link_libraries(alef_cli
                      PRIVATE alefCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/alef/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/alef/lsp/main.cpp)
add_library(alefLsp ${_lsp})
target_link_libraries(alefLsp PUBLIC alefCore cursiv::lsp)
add_executable(alef_lsp alef/lsp/main.cpp)
target_link_libraries(alef_lsp PRIVATE alefLsp)

cursiv_add_benchmark(alef SOURCES alef/bench/main.cpp)

cursiv_package_language(alef)
