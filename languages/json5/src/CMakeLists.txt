# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `json5Generated` (the compiled library) and `json5_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID json5
    GRAMMAR    json5/core/Json5Grammar.hpp
    NAMESPACE  json5::Json5
    EXTENSIONS .json5
    OUT        json5/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/json5/core/*.cpp)
add_library(json5Core ${_core})
target_link_libraries(json5Core PUBLIC json5Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/json5/cli/*.cpp)
add_executable(json5_cli ${_cli})
target_link_libraries(json5_cli
                      PRIVATE json5Core cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/json5/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/json5/lsp/main.cpp)
add_library(json5Lsp ${_lsp})
target_link_libraries(json5Lsp PUBLIC json5Core cursiv::lsp)
add_executable(json5_lsp json5/lsp/main.cpp)
target_link_libraries(json5_lsp PRIVATE json5Lsp)

cursiv_add_benchmark(json5 SOURCES json5/bench/main.cpp)

cursiv_package_language(json5)
