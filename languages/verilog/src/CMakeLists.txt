# Generate the parser and its language module.
cursiv_generate_language(ID verilog
    GRAMMAR    verilog/core/VerilogGrammar.hpp
    NAMESPACE  verilog::Verilog
    EXTENSIONS .v
    OUT        verilog/generated)

# Add custom services and validation over the generated module.
file(GLOB_RECURSE _core CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/verilog/core/*.cpp)
add_library(verilogCore ${_core})
target_link_libraries(verilogCore PUBLIC verilogGenerated)

file(GLOB_RECURSE _cli CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/verilog/cli/*.cpp)
if(_cli)
  add_executable(verilog_cli ${_cli})
  target_link_libraries(verilog_cli
                        PRIVATE verilogCore cursiv::cli)
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/verilog/lsp")
  file(GLOB_RECURSE _lsp CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/verilog/lsp/*.cpp)
  list(REMOVE_ITEM _lsp ${CMAKE_CURRENT_SOURCE_DIR}/verilog/lsp/main.cpp)
  add_library(verilogLsp ${_lsp})
  target_link_libraries(verilogLsp PUBLIC verilogCore cursiv::lsp)
  add_executable(verilog_lsp verilog/lsp/main.cpp)
  target_link_libraries(verilog_lsp PRIVATE verilogLsp)
endif()

if(TARGET verilogLsp AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/verilog/bench/main.cpp")
  cursiv_add_benchmark(verilog
      SOURCES verilog/bench/main.cpp)
endif()

cursiv_package_language(verilog)
