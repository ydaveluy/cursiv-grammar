#include "cursiv/bench/LanguageBench.hpp"
#include "acme/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      acme::AcmeSharedServices, acme::register_lsp_services>(argc, argv);
}
