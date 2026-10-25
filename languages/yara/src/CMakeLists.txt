# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `yaraGenerated` (the compiled library) and `yara_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID yara
    GRAMMAR    yara/core/YaraGrammar.hpp
    NAMESPACE  yara::Yara
    EXTENSIONS .yar
    OUT        yara/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/yara/core/*.cpp)
add_library(yaraCore ${_core})
target_link_libraries(yaraCore PUBLIC yaraGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/yara/cli/*.cpp)
add_executable(yara_cli ${_cli})
target_link_libraries(yara_cli
                      PRIVATE yaraCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/yara/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/yara/lsp/main.cpp)
add_library(yaraLsp ${_lsp})
target_link_libraries(yaraLsp PUBLIC yaraCore cursiv::lsp)
add_executable(yara_lsp yara/lsp/main.cpp)
target_link_libraries(yara_lsp PRIVATE yaraLsp)

cursiv_add_benchmark(yara SOURCES yara/bench/main.cpp)

cursiv_package_language(yara)
