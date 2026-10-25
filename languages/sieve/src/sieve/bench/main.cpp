#include "cursiv/bench/LanguageBench.hpp"
#include "sieve/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      sieve::SieveSharedServices, sieve::register_lsp_services>(argc, argv);
}
