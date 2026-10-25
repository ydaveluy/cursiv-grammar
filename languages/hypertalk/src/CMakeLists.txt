# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `hypertalkGenerated` (the compiled library) and `hypertalk_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID hypertalk
    GRAMMAR    hypertalk/core/HypertalkGrammar.hpp
    NAMESPACE  hypertalk::Hypertalk
    EXTENSIONS .hypertalk
    OUT        hypertalk/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/hypertalk/core/*.cpp)
add_library(hypertalkCore ${_core})
target_link_libraries(hypertalkCore PUBLIC hypertalkGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/hypertalk/cli/*.cpp)
add_executable(hypertalk_cli ${_cli})
target_link_libraries(hypertalk_cli
                      PRIVATE hypertalkCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/hypertalk/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/hypertalk/lsp/main.cpp)
add_library(hypertalkLsp ${_lsp})
target_link_libraries(hypertalkLsp PUBLIC hypertalkCore cursiv::lsp)
add_executable(hypertalk_lsp hypertalk/lsp/main.cpp)
target_link_libraries(hypertalk_lsp PRIVATE hypertalkLsp)

cursiv_add_benchmark(hypertalk SOURCES hypertalk/bench/main.cpp)

cursiv_package_language(hypertalk)
