# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `rustGenerated` (the compiled library) and `rust_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID rust
    GRAMMAR    rust/core/RustGrammar.hpp
    NAMESPACE  rust::Rust
    EXTENSIONS .rs
    OUT        rust/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rust/core/*.cpp)
add_library(rustCore ${_core})
target_link_libraries(rustCore PUBLIC rustGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rust/cli/*.cpp)
add_executable(rust_cli ${_cli})
target_link_libraries(rust_cli
                      PRIVATE rustCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/rust/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/rust/lsp/main.cpp)
add_library(rustLsp ${_lsp})
target_link_libraries(rustLsp PUBLIC rustCore cursiv::lsp)
add_executable(rust_lsp rust/lsp/main.cpp)
target_link_libraries(rust_lsp PRIVATE rustLsp)

cursiv_add_benchmark(rust SOURCES rust/bench/main.cpp)

cursiv_package_language(rust)
