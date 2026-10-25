#include "cursiv/bench/LanguageBench.hpp"
#include "racketbsl/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      racketbsl::RacketBslSharedServices, racketbsl::register_lsp_services>(argc, argv);
}
