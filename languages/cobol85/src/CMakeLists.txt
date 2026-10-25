# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `cobol85Generated` (the compiled library) and `cobol85_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID cobol85
    GRAMMAR    cobol85/core/Cobol85Grammar.hpp
    NAMESPACE  cobol85::Cobol85
    EXTENSIONS .cbl
    OUT        cobol85/generated)


# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cobol85/core/*.cpp)
add_library(cobol85Core ${_core})
target_link_libraries(cobol85Core PUBLIC cobol85Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cobol85/cli/*.cpp)
add_executable(cobol85_cli ${_cli})
target_link_libraries(cobol85_cli
                      PRIVATE cobol85Core cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/cobol85/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/cobol85/lsp/main.cpp)
add_library(cobol85Lsp ${_lsp})
target_link_libraries(cobol85Lsp PUBLIC cobol85Core cursiv::lsp)
add_executable(cobol85_lsp cobol85/lsp/main.cpp)
target_link_libraries(cobol85_lsp PRIVATE cobol85Lsp)

cursiv_add_benchmark(cobol85 SOURCES cobol85/bench/main.cpp)

cursiv_package_language(cobol85)
