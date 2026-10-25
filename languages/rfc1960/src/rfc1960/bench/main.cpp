#include "cursiv/bench/LanguageBench.hpp"
#include "rfc1960/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      rfc1960::Rfc1960SharedServices, rfc1960::register_lsp_services>(argc, argv);
}
