# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `redcodeGenerated` (the compiled library) and `redcode_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID redcode
    GRAMMAR    redcode/core/RedcodeGrammar.hpp
    NAMESPACE  redcode::Redcode
    EXTENSIONS .redcode
    OUT        redcode/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/redcode/core/*.cpp)
add_library(redcodeCore ${_core})
target_link_libraries(redcodeCore PUBLIC redcodeGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/redcode/cli/*.cpp)
add_executable(redcode_cli ${_cli})
target_link_libraries(redcode_cli
                      PRIVATE redcodeCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/redcode/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/redcode/lsp/main.cpp)
add_library(redcodeLsp ${_lsp})
target_link_libraries(redcodeLsp PUBLIC redcodeCore cursiv::lsp)
add_executable(redcode_lsp redcode/lsp/main.cpp)
target_link_libraries(redcode_lsp PRIVATE redcodeLsp)

cursiv_add_benchmark(redcode SOURCES redcode/bench/main.cpp)

cursiv_package_language(redcode)
