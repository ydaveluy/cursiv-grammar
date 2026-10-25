#include "cursiv/bench/LanguageBench.hpp"
#include "modula2pim4/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      modula2pim4::Modula2pim4SharedServices, modula2pim4::register_lsp_services>(argc, argv);
}
