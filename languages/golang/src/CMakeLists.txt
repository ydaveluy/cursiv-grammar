# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `golangGenerated` (the compiled library) and `golang_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID golang
    GRAMMAR    golang/core/GolangGrammar.hpp
    NAMESPACE  golang::Golang
    EXTENSIONS .go
    OUT        golang/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/golang/core/*.cpp)
add_library(golangCore ${_core})
target_link_libraries(golangCore PUBLIC golangGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/golang/cli/*.cpp)
add_executable(golang_cli ${_cli})
target_link_libraries(golang_cli
                      PRIVATE golangCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/golang/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/golang/lsp/main.cpp)
add_library(golangLsp ${_lsp})
target_link_libraries(golangLsp PUBLIC golangCore cursiv::lsp)
add_executable(golang_lsp golang/lsp/main.cpp)
target_link_libraries(golang_lsp PRIVATE golangLsp)

cursiv_add_benchmark(golang SOURCES golang/bench/main.cpp)

cursiv_package_language(golang)
