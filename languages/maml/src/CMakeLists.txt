# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `mamlGenerated` (the compiled library) and `maml_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID maml
    GRAMMAR    maml/core/MamlGrammar.hpp
    NAMESPACE  maml::Maml
    EXTENSIONS .maml
    OUT        maml/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/maml/core/*.cpp)
add_library(mamlCore ${_core})
target_link_libraries(mamlCore PUBLIC mamlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/maml/cli/*.cpp)
add_executable(maml_cli ${_cli})
target_link_libraries(maml_cli
                      PRIVATE mamlCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/maml/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/maml/lsp/main.cpp)
add_library(mamlLsp ${_lsp})
target_link_libraries(mamlLsp PUBLIC mamlCore cursiv::lsp)
add_executable(maml_lsp maml/lsp/main.cpp)
target_link_libraries(maml_lsp PRIVATE mamlLsp)

cursiv_add_benchmark(maml SOURCES maml/bench/main.cpp)

cursiv_package_language(maml)
