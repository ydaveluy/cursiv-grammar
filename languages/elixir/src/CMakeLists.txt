# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `elixirGenerated` (the compiled library) and `elixir_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID elixir
    GRAMMAR    elixir/core/ElixirGrammar.hpp
    NAMESPACE  elixir::Elixir
    EXTENSIONS .ex
    OUT        elixir/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/elixir/core/*.cpp)
add_library(elixirCore ${_core})
target_link_libraries(elixirCore PUBLIC elixirGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/elixir/cli/*.cpp)
add_executable(elixir_cli ${_cli})
target_link_libraries(elixir_cli
                      PRIVATE elixirCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/elixir/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/elixir/lsp/main.cpp)
add_library(elixirLsp ${_lsp})
target_link_libraries(elixirLsp PUBLIC elixirCore cursiv::lsp)
add_executable(elixir_lsp elixir/lsp/main.cpp)
target_link_libraries(elixir_lsp PRIVATE elixirLsp)

cursiv_add_benchmark(elixir SOURCES elixir/bench/main.cpp)

cursiv_package_language(elixir)
