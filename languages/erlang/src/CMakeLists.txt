# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `erlangGenerated` (the compiled library) and `erlang_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID erlang
    GRAMMAR    erlang/core/ErlangGrammar.hpp
    NAMESPACE  erlang::Erlang
    EXTENSIONS .erl
    OUT        erlang/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/erlang/core/*.cpp)
add_library(erlangCore ${_core})
target_link_libraries(erlangCore PUBLIC erlangGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/erlang/cli/*.cpp)
add_executable(erlang_cli ${_cli})
target_link_libraries(erlang_cli
                      PRIVATE erlangCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/erlang/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/erlang/lsp/main.cpp)
add_library(erlangLsp ${_lsp})
target_link_libraries(erlangLsp PUBLIC erlangCore cursiv::lsp)
add_executable(erlang_lsp erlang/lsp/main.cpp)
target_link_libraries(erlang_lsp PRIVATE erlangLsp)

cursiv_add_benchmark(erlang SOURCES erlang/bench/main.cpp)

cursiv_package_language(erlang)
