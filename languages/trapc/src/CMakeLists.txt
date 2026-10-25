# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `trapcGenerated` (the compiled library) and `trapc_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID trapc
    GRAMMAR    trapc/core/TrapcGrammar.hpp
    NAMESPACE  trapc::Trapc
    EXTENSIONS .tc
    OUT        trapc/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/trapc/core/*.cpp)
add_library(trapcCore ${_core})
target_link_libraries(trapcCore PUBLIC trapcGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/trapc/cli/*.cpp)
add_executable(trapc_cli ${_cli})
target_link_libraries(trapc_cli
                      PRIVATE trapcCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/trapc/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/trapc/lsp/main.cpp)
add_library(trapcLsp ${_lsp})
target_link_libraries(trapcLsp PUBLIC trapcCore cursiv::lsp)
add_executable(trapc_lsp trapc/lsp/main.cpp)
target_link_libraries(trapc_lsp PRIVATE trapcLsp)

cursiv_add_benchmark(trapc SOURCES trapc/bench/main.cpp)

cursiv_package_language(trapc)
