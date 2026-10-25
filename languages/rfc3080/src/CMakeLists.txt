# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `rfc3080Generated` (the compiled library) and `rfc3080_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID rfc3080
    GRAMMAR    rfc3080/core/Rfc3080Grammar.hpp
    NAMESPACE  rfc3080::Rfc3080
    EXTENSIONS .beep
    OUT        rfc3080/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rfc3080/core/*.cpp)
add_library(rfc3080Core ${_core})
target_link_libraries(rfc3080Core PUBLIC rfc3080Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rfc3080/cli/*.cpp)
add_executable(rfc3080_cli ${_cli})
target_link_libraries(rfc3080_cli
                      PRIVATE rfc3080Core cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rfc3080/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/rfc3080/lsp/main.cpp)
add_library(rfc3080Lsp ${_lsp})
target_link_libraries(rfc3080Lsp PUBLIC rfc3080Core cursiv::lsp)
add_executable(rfc3080_lsp rfc3080/lsp/main.cpp)
target_link_libraries(rfc3080_lsp PRIVATE rfc3080Lsp)

cursiv_add_benchmark(rfc3080 SOURCES rfc3080/bench/main.cpp)

cursiv_package_language(rfc3080)
