#include "cursiv/LanguageBench.hpp"
#include "racketbsl/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      racketbsl::RacketBslSharedServices, racketbsl::register_lsp_services>(argc, argv);
}
