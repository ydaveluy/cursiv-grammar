#include "cursiv/LanguageBench.hpp"
#include "verilog/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      verilog::VerilogSharedServices, verilog::register_lsp_services>(argc, argv);
}
