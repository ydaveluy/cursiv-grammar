# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `alpacaGenerated` (the compiled library) and `alpaca_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID alpaca
    GRAMMAR    alpaca/core/AlpacaGrammar.hpp
    NAMESPACE  alpaca::Alpaca
    EXTENSIONS .alp
    OUT        alpaca/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/alpaca/core/*.cpp)
add_library(alpacaCore ${_core})
target_link_libraries(alpacaCore PUBLIC alpacaGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/alpaca/cli/*.cpp)
add_executable(alpaca_cli ${_cli})
target_link_libraries(alpaca_cli
                      PRIVATE alpacaCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/alpaca/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/alpaca/lsp/main.cpp)
add_library(alpacaLsp ${_lsp})
target_link_libraries(alpacaLsp PUBLIC alpacaCore cursiv::lsp)
add_executable(alpaca_lsp alpaca/lsp/main.cpp)
target_link_libraries(alpaca_lsp PRIVATE alpacaLsp)

cursiv_add_benchmark(alpaca SOURCES alpaca/bench/main.cpp)

cursiv_package_language(alpaca)
