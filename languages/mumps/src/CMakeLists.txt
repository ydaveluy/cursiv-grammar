# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `mumpsGenerated` (the compiled library) and `mumps_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID mumps
    GRAMMAR    mumps/core/MumpsGrammar.hpp
    NAMESPACE  mumps::Mumps
    EXTENSIONS .m
    OUT        mumps/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/mumps/core/*.cpp)
add_library(mumpsCore ${_core})
target_link_libraries(mumpsCore PUBLIC mumpsGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/mumps/cli/*.cpp)
add_executable(mumps_cli ${_cli})
target_link_libraries(mumps_cli
                      PRIVATE mumpsCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/mumps/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/mumps/lsp/main.cpp)
add_library(mumpsLsp ${_lsp})
target_link_libraries(mumpsLsp PUBLIC mumpsCore cursiv::lsp)
add_executable(mumps_lsp mumps/lsp/main.cpp)
target_link_libraries(mumps_lsp PRIVATE mumpsLsp)

cursiv_add_benchmark(mumps SOURCES mumps/bench/main.cpp)

cursiv_package_language(mumps)
