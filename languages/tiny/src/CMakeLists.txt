# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `tinyGenerated` (the compiled library) and `tiny_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID tiny
    GRAMMAR    tiny/core/TinyGrammar.hpp
    NAMESPACE  tiny::Tiny
    EXTENSIONS .tiny
    OUT        tiny/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tiny/core/*.cpp)
add_library(tinyCore ${_core})
target_link_libraries(tinyCore PUBLIC tinyGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tiny/cli/*.cpp)
add_executable(tiny_cli ${_cli})
target_link_libraries(tiny_cli
                      PRIVATE tinyCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tiny/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/tiny/lsp/main.cpp)
add_library(tinyLsp ${_lsp})
target_link_libraries(tinyLsp PUBLIC tinyCore cursiv::lsp)
add_executable(tiny_lsp tiny/lsp/main.cpp)
target_link_libraries(tiny_lsp PRIVATE tinyLsp)

cursiv_add_benchmark(tiny SOURCES tiny/bench/main.cpp)

cursiv_package_language(tiny)
