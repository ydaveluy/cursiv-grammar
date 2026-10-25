#include "cursiv/bench/LanguageBench.hpp"
#include "racketisl/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      racketisl::RacketIslSharedServices, racketisl::register_lsp_services>(argc, argv);
}
