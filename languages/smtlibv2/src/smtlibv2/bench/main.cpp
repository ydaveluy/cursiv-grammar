#include "cursiv/bench/LanguageBench.hpp"
#include "smtlibv2/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      smtlibv2::Smtlibv2SharedServices, smtlibv2::register_lsp_services>(argc, argv);
}
