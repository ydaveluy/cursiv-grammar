# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `awkGenerated` (the compiled library) and `awk_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID awk
    GRAMMAR    awk/core/AwkGrammar.hpp
    NAMESPACE  awk::Awk
    EXTENSIONS .awk
    OUT        awk/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/awk/core/*.cpp)
add_library(awkCore ${_core})
target_link_libraries(awkCore PUBLIC awkGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/awk/cli/*.cpp)
add_executable(awk_cli ${_cli})
target_link_libraries(awk_cli
                      PRIVATE awkCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/awk/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/awk/lsp/main.cpp)
add_library(awkLsp ${_lsp})
target_link_libraries(awkLsp PUBLIC awkCore cursiv::lsp)
add_executable(awk_lsp awk/lsp/main.cpp)
target_link_libraries(awk_lsp PRIVATE awkLsp)

cursiv_add_benchmark(awk SOURCES awk/bench/main.cpp)

cursiv_package_language(awk)
