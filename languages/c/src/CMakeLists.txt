# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `cGenerated` (the compiled library) and `c_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID c
    GRAMMAR    c/core/CGrammar.hpp
    NAMESPACE  c::C
    EXTENSIONS .c .h
    OUT        c/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/c/core/*.cpp)
add_library(cCore ${_core})
target_link_libraries(cCore PUBLIC cGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/c/cli/*.cpp)
add_executable(c_cli ${_cli})
target_link_libraries(c_cli
                      PRIVATE cCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/c/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/c/lsp/main.cpp)
add_library(cLsp ${_lsp})
target_link_libraries(cLsp PUBLIC cCore cursiv::lsp)
add_executable(c_lsp c/lsp/main.cpp)
target_link_libraries(c_lsp PRIVATE cLsp)

cursiv_add_benchmark(c SOURCES c/bench/main.cpp)

cursiv_package_language(c)
