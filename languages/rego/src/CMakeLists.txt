# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `regoGenerated` (the compiled library) and `rego_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID rego
    GRAMMAR    rego/core/RegoGrammar.hpp
    NAMESPACE  rego::Rego
    EXTENSIONS .rego
    OUT        rego/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rego/core/*.cpp)
add_library(regoCore ${_core})
target_link_libraries(regoCore PUBLIC regoGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rego/cli/*.cpp)
add_executable(rego_cli ${_cli})
target_link_libraries(rego_cli
                      PRIVATE regoCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rego/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/rego/lsp/main.cpp)
add_library(regoLsp ${_lsp})
target_link_libraries(regoLsp PUBLIC regoCore cursiv::lsp)
add_executable(rego_lsp rego/lsp/main.cpp)
target_link_libraries(rego_lsp PRIVATE regoLsp)

cursiv_add_benchmark(rego SOURCES rego/bench/main.cpp)

cursiv_package_language(rego)
