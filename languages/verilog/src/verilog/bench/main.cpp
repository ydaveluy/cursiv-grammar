#include "cursiv/bench/LanguageBench.hpp"
#include "verilog/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      verilog::VerilogSharedServices, verilog::register_lsp_services>(argc, argv);
}
