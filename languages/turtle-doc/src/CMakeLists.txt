# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `turtledocGenerated` (the compiled library) and `turtledoc_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID turtledoc
    GRAMMAR    turtledoc/core/TurtleDocGrammar.hpp
    NAMESPACE  turtledoc::TurtleDoc
    EXTENSIONS .ttl
    OUT        turtledoc/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/turtledoc/core/*.cpp)
add_library(turtledocCore ${_core})
target_link_libraries(turtledocCore PUBLIC turtledocGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/turtledoc/cli/*.cpp)
add_executable(turtledoc_cli ${_cli})
target_link_libraries(turtledoc_cli
                      PRIVATE turtledocCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/turtledoc/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/turtledoc/lsp/main.cpp)
add_library(turtledocLsp ${_lsp})
target_link_libraries(turtledocLsp PUBLIC turtledocCore cursiv::lsp)
add_executable(turtledoc_lsp turtledoc/lsp/main.cpp)
target_link_libraries(turtledoc_lsp PRIVATE turtledocLsp)

cursiv_add_benchmark(turtledoc SOURCES turtledoc/bench/main.cpp)

cursiv_package_language(turtledoc)
