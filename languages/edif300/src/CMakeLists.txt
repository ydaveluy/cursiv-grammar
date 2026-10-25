# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `edif300Generated` (the compiled library) and `edif300_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID edif300
    GRAMMAR    edif300/core/Edif300Grammar.hpp
    NAMESPACE  edif300::Edif300
    EXTENSIONS .edf
    OUT        edif300/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/edif300/core/*.cpp)
add_library(edif300Core ${_core})
target_link_libraries(edif300Core PUBLIC edif300Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/edif300/cli/*.cpp)
add_executable(edif300_cli ${_cli})
target_link_libraries(edif300_cli
                      PRIVATE edif300Core cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/edif300/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/edif300/lsp/main.cpp)
add_library(edif300Lsp ${_lsp})
target_link_libraries(edif300Lsp PUBLIC edif300Core cursiv::lsp)
add_executable(edif300_lsp edif300/lsp/main.cpp)
target_link_libraries(edif300_lsp PRIVATE edif300Lsp)

cursiv_add_benchmark(edif300 SOURCES edif300/bench/main.cpp)

cursiv_package_language(edif300)
