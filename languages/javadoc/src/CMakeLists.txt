# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `javadocGenerated` (the compiled library) and `javadoc_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID javadoc
    GRAMMAR    javadoc/core/JavadocGrammar.hpp
    NAMESPACE  javadoc::Javadoc
    EXTENSIONS .javadoc
    OUT        javadoc/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/javadoc/core/*.cpp)
add_library(javadocCore ${_core})
target_link_libraries(javadocCore PUBLIC javadocGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/javadoc/cli/*.cpp)
add_executable(javadoc_cli ${_cli})
target_link_libraries(javadoc_cli
                      PRIVATE javadocCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/javadoc/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/javadoc/lsp/main.cpp)
add_library(javadocLsp ${_lsp})
target_link_libraries(javadocLsp PUBLIC javadocCore cursiv::lsp)
add_executable(javadoc_lsp javadoc/lsp/main.cpp)
target_link_libraries(javadoc_lsp PRIVATE javadocLsp)

cursiv_add_benchmark(javadoc SOURCES javadoc/bench/main.cpp)

cursiv_package_language(javadoc)
