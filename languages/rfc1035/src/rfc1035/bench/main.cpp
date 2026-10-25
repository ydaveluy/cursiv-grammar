#include "cursiv/bench/LanguageBench.hpp"
#include "rfc1035/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      rfc1035::Rfc1035SharedServices, rfc1035::register_lsp_services>(argc, argv);
}
