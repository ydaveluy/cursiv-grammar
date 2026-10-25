# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `scalaGenerated` (the compiled library) and `scala_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID scala
    GRAMMAR    scala/core/ScalaGrammar.hpp
    NAMESPACE  scala::Scala
    EXTENSIONS .scala
    OUT        scala/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/scala/core/*.cpp)
add_library(scalaCore ${_core})
target_link_libraries(scalaCore PUBLIC scalaGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/scala/cli/*.cpp)
add_executable(scala_cli ${_cli})
target_link_libraries(scala_cli
                      PRIVATE scalaCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/scala/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/scala/lsp/main.cpp)
add_library(scalaLsp ${_lsp})
target_link_libraries(scalaLsp PUBLIC scalaCore cursiv::lsp)
add_executable(scala_lsp scala/lsp/main.cpp)
target_link_libraries(scala_lsp PRIVATE scalaLsp)

cursiv_add_benchmark(scala SOURCES scala/bench/main.cpp)

cursiv_package_language(scala)
