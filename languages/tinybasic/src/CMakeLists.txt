# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `tinybasicGenerated` (the compiled library) and `tinybasic_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID tinybasic
    GRAMMAR    tinybasic/core/TinybasicGrammar.hpp
    NAMESPACE  tinybasic::Tinybasic
    EXTENSIONS .tinybasic
    OUT        tinybasic/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tinybasic/core/*.cpp)
add_library(tinybasicCore ${_core})
target_link_libraries(tinybasicCore PUBLIC tinybasicGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tinybasic/cli/*.cpp)
add_executable(tinybasic_cli ${_cli})
target_link_libraries(tinybasic_cli
                      PRIVATE tinybasicCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/tinybasic/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/tinybasic/lsp/main.cpp)
add_library(tinybasicLsp ${_lsp})
target_link_libraries(tinybasicLsp PUBLIC tinybasicCore cursiv::lsp)
add_executable(tinybasic_lsp tinybasic/lsp/main.cpp)
target_link_libraries(tinybasic_lsp PRIVATE tinybasicLsp)

cursiv_add_benchmark(tinybasic SOURCES tinybasic/bench/main.cpp)

cursiv_package_language(tinybasic)
