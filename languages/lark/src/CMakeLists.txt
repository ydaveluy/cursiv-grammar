# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `larkGenerated` (the compiled library) and `lark_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID lark
    GRAMMAR    lark/core/LarkGrammar.hpp
    NAMESPACE  lark::Lark
    EXTENSIONS .lark
    OUT        lark/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lark/core/*.cpp)
add_library(larkCore ${_core})
target_link_libraries(larkCore PUBLIC larkGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lark/cli/*.cpp)
add_executable(lark_cli ${_cli})
target_link_libraries(lark_cli
                      PRIVATE larkCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lark/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/lark/lsp/main.cpp)
add_library(larkLsp ${_lsp})
target_link_libraries(larkLsp PUBLIC larkCore cursiv::lsp)
add_executable(lark_lsp lark/lsp/main.cpp)
target_link_libraries(lark_lsp PRIVATE larkLsp)

cursiv_add_benchmark(lark SOURCES lark/bench/main.cpp)

cursiv_package_language(lark)
