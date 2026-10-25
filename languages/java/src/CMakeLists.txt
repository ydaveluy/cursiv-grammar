# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `javaGenerated` (the compiled library) and `java_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID java
    GRAMMAR    java/core/JavaGrammar.hpp
    NAMESPACE  java::Java
    EXTENSIONS .java
    OUT        java/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/java/core/*.cpp)
add_library(javaCore ${_core})
target_link_libraries(javaCore PUBLIC javaGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/java/cli/*.cpp)
add_executable(java_cli ${_cli})
target_link_libraries(java_cli
                      PRIVATE javaCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/java/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/java/lsp/main.cpp)
add_library(javaLsp ${_lsp})
target_link_libraries(javaLsp PUBLIC javaCore cursiv::lsp)
add_executable(java_lsp java/lsp/main.cpp)
target_link_libraries(java_lsp PRIVATE javaLsp)

cursiv_add_benchmark(java SOURCES java/bench/main.cpp)

cursiv_package_language(java)
