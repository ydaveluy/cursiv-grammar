# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `geekcodeGenerated` (the compiled library) and `geekcode_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID geekcode
    GRAMMAR    geekcode/core/GeekcodeGrammar.hpp
    NAMESPACE  geekcode::Geekcode
    EXTENSIONS .geek
    OUT        geekcode/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/geekcode/core/*.cpp)
add_library(geekcodeCore ${_core})
target_link_libraries(geekcodeCore PUBLIC geekcodeGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/geekcode/cli/*.cpp)
add_executable(geekcode_cli ${_cli})
target_link_libraries(geekcode_cli
                      PRIVATE geekcodeCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/geekcode/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/geekcode/lsp/main.cpp)
add_library(geekcodeLsp ${_lsp})
target_link_libraries(geekcodeLsp PUBLIC geekcodeCore cursiv::lsp)
add_executable(geekcode_lsp geekcode/lsp/main.cpp)
target_link_libraries(geekcode_lsp PRIVATE geekcodeLsp)

cursiv_add_benchmark(geekcode SOURCES geekcode/bench/main.cpp)

cursiv_package_language(geekcode)
