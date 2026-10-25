# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `tinymudGenerated` (the compiled library) and `tinymud_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID tinymud
    GRAMMAR    tinymud/core/TinymudGrammar.hpp
    NAMESPACE  tinymud::Tinymud
    EXTENSIONS .tinymud
    OUT        tinymud/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tinymud/core/*.cpp)
add_library(tinymudCore ${_core})
target_link_libraries(tinymudCore PUBLIC tinymudGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tinymud/cli/*.cpp)
add_executable(tinymud_cli ${_cli})
target_link_libraries(tinymud_cli
                      PRIVATE tinymudCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tinymud/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/tinymud/lsp/main.cpp)
add_library(tinymudLsp ${_lsp})
target_link_libraries(tinymudLsp PUBLIC tinymudCore cursiv::lsp)
add_executable(tinymud_lsp tinymud/lsp/main.cpp)
target_link_libraries(tinymud_lsp PRIVATE tinymudLsp)

cursiv_add_benchmark(tinymud SOURCES tinymud/bench/main.cpp)

cursiv_package_language(tinymud)
