# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `lolcodeGenerated` (the compiled library) and `lolcode_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID lolcode
    GRAMMAR    lolcode/core/LolcodeGrammar.hpp
    NAMESPACE  lolcode::Lolcode
    EXTENSIONS .lol
    OUT        lolcode/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lolcode/core/*.cpp)
add_library(lolcodeCore ${_core})
target_link_libraries(lolcodeCore PUBLIC lolcodeGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lolcode/cli/*.cpp)
add_executable(lolcode_cli ${_cli})
target_link_libraries(lolcode_cli
                      PRIVATE lolcodeCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/lolcode/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/lolcode/lsp/main.cpp)
add_library(lolcodeLsp ${_lsp})
target_link_libraries(lolcodeLsp PUBLIC lolcodeCore cursiv::lsp)
add_executable(lolcode_lsp lolcode/lsp/main.cpp)
target_link_libraries(lolcode_lsp PRIVATE lolcodeLsp)

cursiv_add_benchmark(lolcode SOURCES lolcode/bench/main.cpp)

cursiv_package_language(lolcode)
