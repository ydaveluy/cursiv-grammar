#include "cursiv/bench/LanguageBench.hpp"
#include "iso8601/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      iso8601::Iso8601SharedServices, iso8601::register_lsp_services>(argc, argv);
}
