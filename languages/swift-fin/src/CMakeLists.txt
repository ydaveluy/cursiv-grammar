# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `swiftfinGenerated` (the compiled library) and `swiftfin_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID swiftfin
    GRAMMAR    swiftfin/core/SwiftFinGrammar.hpp
    NAMESPACE  swiftfin::SwiftFin
    EXTENSIONS .swiftfin
    OUT        swiftfin/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/swiftfin/core/*.cpp)
add_library(swiftfinCore ${_core})
target_link_libraries(swiftfinCore PUBLIC swiftfinGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/swiftfin/cli/*.cpp)
add_executable(swiftfin_cli ${_cli})
target_link_libraries(swiftfin_cli
                      PRIVATE swiftfinCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/swiftfin/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/swiftfin/lsp/main.cpp)
add_library(swiftfinLsp ${_lsp})
target_link_libraries(swiftfinLsp PUBLIC swiftfinCore cursiv::lsp)
add_executable(swiftfin_lsp swiftfin/lsp/main.cpp)
target_link_libraries(swiftfin_lsp PRIVATE swiftfinLsp)

cursiv_add_benchmark(swiftfin SOURCES swiftfin/bench/main.cpp)

cursiv_package_language(swiftfin)
