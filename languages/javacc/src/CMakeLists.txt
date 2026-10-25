# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `javaccGenerated` (the compiled library) and `javacc_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID javacc
    GRAMMAR    javacc/core/JavaccGrammar.hpp
    NAMESPACE  javacc::Javacc
    EXTENSIONS .jj .jjt
    OUT        javacc/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/javacc/core/*.cpp)
add_library(javaccCore ${_core})
target_link_libraries(javaccCore PUBLIC javaccGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/javacc/cli/*.cpp)
add_executable(javacc_cli ${_cli})
target_link_libraries(javacc_cli
                      PRIVATE javaccCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/javacc/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/javacc/lsp/main.cpp)
add_library(javaccLsp ${_lsp})
target_link_libraries(javaccLsp PUBLIC javaccCore cursiv::lsp)
add_executable(javacc_lsp javacc/lsp/main.cpp)
target_link_libraries(javacc_lsp PRIVATE javaccLsp)

cursiv_add_benchmark(javacc SOURCES javacc/bench/main.cpp)

cursiv_package_language(javacc)
