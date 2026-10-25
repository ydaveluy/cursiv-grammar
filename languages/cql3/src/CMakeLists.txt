# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `cql3Generated` (the compiled library) and `cql3_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID cql3
    GRAMMAR    cql3/core/Cql3Grammar.hpp
    NAMESPACE  cql3::Cql3
    EXTENSIONS .cql3
    OUT        cql3/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cql3/core/*.cpp)
add_library(cql3Core ${_core})
target_link_libraries(cql3Core PUBLIC cql3Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cql3/cli/*.cpp)
add_executable(cql3_cli ${_cli})
target_link_libraries(cql3_cli
                      PRIVATE cql3Core cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cql3/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/cql3/lsp/main.cpp)
add_library(cql3Lsp ${_lsp})
target_link_libraries(cql3Lsp PUBLIC cql3Core cursiv::lsp)
add_executable(cql3_lsp cql3/lsp/main.cpp)
target_link_libraries(cql3_lsp PRIVATE cql3Lsp)

cursiv_add_benchmark(cql3 SOURCES cql3/bench/main.cpp)

cursiv_package_language(cql3)
