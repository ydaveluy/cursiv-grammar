# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `vhdlGenerated` (the compiled library) and `vhdl_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID vhdl
    GRAMMAR    vhdl/core/VhdlGrammar.hpp
    NAMESPACE  vhdl::Vhdl
    EXTENSIONS .vhd
    OUT        vhdl/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/vhdl/core/*.cpp)
add_library(vhdlCore ${_core})
target_link_libraries(vhdlCore PUBLIC vhdlGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/vhdl/cli/*.cpp)
add_executable(vhdl_cli ${_cli})
target_link_libraries(vhdl_cli
                      PRIVATE vhdlCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/vhdl/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/vhdl/lsp/main.cpp)
add_library(vhdlLsp ${_lsp})
target_link_libraries(vhdlLsp PUBLIC vhdlCore cursiv::lsp)
add_executable(vhdl_lsp vhdl/lsp/main.cpp)
target_link_libraries(vhdl_lsp PRIVATE vhdlLsp)

cursiv_add_benchmark(vhdl SOURCES vhdl/bench/main.cpp)

cursiv_package_language(vhdl)
