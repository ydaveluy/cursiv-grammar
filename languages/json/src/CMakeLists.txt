# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `jsonGenerated` (the compiled library) and `json_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID json
    GRAMMAR    json/core/JsonGrammar.hpp
    NAMESPACE  json::Json
    EXTENSIONS .json
    OUT        json/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/json/core/*.cpp)
add_library(jsonCore ${_core})
target_link_libraries(jsonCore PUBLIC jsonGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/json/cli/*.cpp)
add_executable(json_cli ${_cli})
target_link_libraries(json_cli
                      PRIVATE jsonCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/json/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/json/lsp/main.cpp)
add_library(jsonLsp ${_lsp})
target_link_libraries(jsonLsp PUBLIC jsonCore cursiv::lsp)
add_executable(json_lsp json/lsp/main.cpp)
target_link_libraries(json_lsp PRIVATE jsonLsp)

cursiv_add_benchmark(json SOURCES json/bench/main.cpp)

cursiv_package_language(json)
