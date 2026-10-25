# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `kotlinGenerated` (the compiled library) and `kotlin_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID kotlin
    GRAMMAR    kotlin/core/KotlinGrammar.hpp
    NAMESPACE  kotlin::Kotlin
    EXTENSIONS .kt
    OUT        kotlin/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/kotlin/core/*.cpp)
add_library(kotlinCore ${_core})
target_link_libraries(kotlinCore PUBLIC kotlinGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/kotlin/cli/*.cpp)
add_executable(kotlin_cli ${_cli})
target_link_libraries(kotlin_cli
                      PRIVATE kotlinCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/kotlin/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/kotlin/lsp/main.cpp)
add_library(kotlinLsp ${_lsp})
target_link_libraries(kotlinLsp PUBLIC kotlinCore cursiv::lsp)
add_executable(kotlin_lsp kotlin/lsp/main.cpp)
target_link_libraries(kotlin_lsp PRIVATE kotlinLsp)

cursiv_add_benchmark(kotlin SOURCES kotlin/bench/main.cpp)

cursiv_package_language(kotlin)
