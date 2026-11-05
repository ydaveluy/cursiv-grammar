#include "cursiv/LanguageBench.hpp"
#include "vhdl/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      vhdl::VhdlSharedServices, vhdl::register_lsp_services>(argc, argv);
}
