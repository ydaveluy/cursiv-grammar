# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `iso8601Generated` (the compiled library) and `iso8601_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID iso8601
    GRAMMAR    iso8601/core/Iso8601Grammar.hpp
    NAMESPACE  iso8601::Iso8601
    EXTENSIONS .iso8601
    OUT        iso8601/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/iso8601/core/*.cpp)
add_library(iso8601Core ${_core})
target_link_libraries(iso8601Core PUBLIC iso8601Generated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/iso8601/cli/*.cpp)
add_executable(iso8601_cli ${_cli})
target_link_libraries(iso8601_cli
                      PRIVATE iso8601Core cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/iso8601/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/iso8601/lsp/main.cpp)
add_library(iso8601Lsp ${_lsp})
target_link_libraries(iso8601Lsp PUBLIC iso8601Core cursiv::lsp)
add_executable(iso8601_lsp iso8601/lsp/main.cpp)
target_link_libraries(iso8601_lsp PRIVATE iso8601Lsp)

cursiv_add_benchmark(iso8601 SOURCES iso8601/bench/main.cpp)

cursiv_package_language(iso8601)
