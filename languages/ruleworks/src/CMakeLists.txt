# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `ruleworksGenerated` (the compiled library) and `ruleworks_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID ruleworks
    GRAMMAR    ruleworks/core/RuleworksGrammar.hpp
    NAMESPACE  ruleworks::Ruleworks
    EXTENSIONS .rul
    OUT        ruleworks/generated)

# The grammar's write-action table is large enough (~150 terminals, ~150
# rules) to need more than the compiler's default template recursion depth.
target_compile_options(ruleworks_gen PRIVATE -ftemplate-depth=4000)
target_compile_options(ruleworksGenerated PRIVATE -ftemplate-depth=4000)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ruleworks/core/*.cpp)
add_library(ruleworksCore ${_core})
target_link_libraries(ruleworksCore PUBLIC ruleworksGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ruleworks/cli/*.cpp)
add_executable(ruleworks_cli ${_cli})
target_link_libraries(ruleworks_cli
                      PRIVATE ruleworksCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/ruleworks/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/ruleworks/lsp/main.cpp)
add_library(ruleworksLsp ${_lsp})
target_link_libraries(ruleworksLsp PUBLIC ruleworksCore cursiv::lsp)
add_executable(ruleworks_lsp ruleworks/lsp/main.cpp)
target_link_libraries(ruleworks_lsp PRIVATE ruleworksLsp)

cursiv_add_benchmark(ruleworks SOURCES ruleworks/bench/main.cpp)

cursiv_package_language(ruleworks)
