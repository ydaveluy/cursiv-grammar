# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `abnfGenerated` (the compiled library) and `abnf_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID abnf
    GRAMMAR    abnf/core/AbnfGrammar.hpp
    NAMESPACE  abnf::Abnf
    EXTENSIONS .abnf
    OUT        abnf/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/abnf/core/*.cpp)
add_library(abnfCore ${_core})
target_link_libraries(abnfCore PUBLIC abnfGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/abnf/cli/*.cpp)
add_executable(abnf_cli ${_cli})
target_link_libraries(abnf_cli
                      PRIVATE abnfCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/abnf/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/abnf/lsp/main.cpp)
add_library(abnfLsp ${_lsp})
target_link_libraries(abnfLsp PUBLIC abnfCore cursiv::lsp)
add_executable(abnf_lsp abnf/lsp/main.cpp)
target_link_libraries(abnf_lsp PRIVATE abnfLsp)

cursiv_add_benchmark(abnf SOURCES abnf/bench/main.cpp)

cursiv_package_language(abnf)
