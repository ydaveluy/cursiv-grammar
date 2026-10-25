# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `peoplecodeGenerated` (the compiled library) and `peoplecode_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID peoplecode
    GRAMMAR    peoplecode/core/PeoplecodeGrammar.hpp
    NAMESPACE  peoplecode::Peoplecode
    EXTENSIONS .pc
    OUT        peoplecode/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/peoplecode/core/*.cpp)
add_library(peoplecodeCore ${_core})
target_link_libraries(peoplecodeCore PUBLIC peoplecodeGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/peoplecode/cli/*.cpp)
add_executable(peoplecode_cli ${_cli})
target_link_libraries(peoplecode_cli
                      PRIVATE peoplecodeCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/peoplecode/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/peoplecode/lsp/main.cpp)
add_library(peoplecodeLsp ${_lsp})
target_link_libraries(peoplecodeLsp PUBLIC peoplecodeCore cursiv::lsp)
add_executable(peoplecode_lsp peoplecode/lsp/main.cpp)
target_link_libraries(peoplecode_lsp PRIVATE peoplecodeLsp)

cursiv_add_benchmark(peoplecode SOURCES peoplecode/bench/main.cpp)

cursiv_package_language(peoplecode)
