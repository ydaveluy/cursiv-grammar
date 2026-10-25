# `ID` names the targets and generated directory; `NAMESPACE` names the
# grammar type, and everything else follows from it. Defines
# `verilogGenerated` (the compiled library) and `verilog_generate`
# (writes the sources without compiling them).
cursiv_generate_language(ID verilog
    GRAMMAR    verilog/core/VerilogGrammar.hpp
    NAMESPACE  verilog::Verilog
    EXTENSIONS .v
    OUT        verilog/generated)

# The core storey: what the author writes over the generated module -- the
# services container and the validation checks. The grammar is not compiled
# here; the generated module holds everything derived from it.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/verilog/core/*.cpp)
add_library(verilogCore ${_core})
target_link_libraries(verilogCore PUBLIC verilogGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/verilog/cli/*.cpp)
add_executable(verilog_cli ${_cli})
target_link_libraries(verilog_cli
                      PRIVATE verilogCore cursiv::cli)

# The language server.
file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/verilog/lsp/*.cpp)
list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/verilog/lsp/main.cpp)
add_library(verilogLsp ${_lsp})
target_link_libraries(verilogLsp PUBLIC verilogCore cursiv::lsp)
add_executable(verilog_lsp verilog/lsp/main.cpp)
target_link_libraries(verilog_lsp PRIVATE verilogLsp)

cursiv_add_benchmark(verilog SOURCES verilog/bench/main.cpp)

cursiv_package_language(verilog)
