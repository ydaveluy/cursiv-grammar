# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `zigGenerated` (the compiled library) and `zig_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID zig
    GRAMMAR    zig/core/ZigGrammar.hpp
    NAMESPACE  zig::Zig
    EXTENSIONS .zig
    OUT        zig/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/zig/core/*.cpp)
add_library(zigCore ${_core})
target_link_libraries(zigCore PUBLIC zigGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/zig/cli/*.cpp)
add_executable(zig_cli ${_cli})
target_link_libraries(zig_cli
                      PRIVATE zigCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/zig/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/zig/lsp/main.cpp)
add_library(zigLsp ${_lsp})
target_link_libraries(zigLsp PUBLIC zigCore cursiv::lsp)
add_executable(zig_lsp zig/lsp/main.cpp)
target_link_libraries(zig_lsp PRIVATE zigLsp)

cursiv_add_benchmark(zig SOURCES zig/bench/main.cpp)

cursiv_package_language(zig)
