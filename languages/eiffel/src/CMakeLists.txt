# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `eiffelGenerated` (the compiled library) and `eiffel_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID eiffel
    GRAMMAR    eiffel/core/EiffelGrammar.hpp
    NAMESPACE  eiffel::Eiffel
    EXTENSIONS .e
    OUT        eiffel/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/eiffel/core/*.cpp)
add_library(eiffelCore ${_core})
target_link_libraries(eiffelCore PUBLIC eiffelGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/eiffel/cli/*.cpp)
add_executable(eiffel_cli ${_cli})
target_link_libraries(eiffel_cli
                      PRIVATE eiffelCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/eiffel/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/eiffel/lsp/main.cpp)
add_library(eiffelLsp ${_lsp})
target_link_libraries(eiffelLsp PUBLIC eiffelCore cursiv::lsp)
add_executable(eiffel_lsp eiffel/lsp/main.cpp)
target_link_libraries(eiffel_lsp PRIVATE eiffelLsp)

cursiv_add_benchmark(eiffel SOURCES eiffel/bench/main.cpp)

cursiv_package_language(eiffel)
