# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `typescriptGenerated` (the compiled library) and `typescript_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID typescript
    GRAMMAR    typescript/core/TypeScriptGrammar.hpp
    NAMESPACE  typescript::TypeScript
    EXTENSIONS .ts
    OUT        typescript/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/typescript/core/*.cpp)
add_library(typescriptCore ${_core})
target_link_libraries(typescriptCore PUBLIC typescriptGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/typescript/cli/*.cpp)
add_executable(typescript_cli ${_cli})
target_link_libraries(typescript_cli
                      PRIVATE typescriptCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/typescript/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/typescript/lsp/main.cpp)
add_library(typescriptLsp ${_lsp})
target_link_libraries(typescriptLsp PUBLIC typescriptCore cursiv::lsp)
add_executable(typescript_lsp typescript/lsp/main.cpp)
target_link_libraries(typescript_lsp PRIVATE typescriptLsp)

cursiv_add_benchmark(typescript SOURCES typescript/bench/main.cpp)

cursiv_package_language(typescript)
