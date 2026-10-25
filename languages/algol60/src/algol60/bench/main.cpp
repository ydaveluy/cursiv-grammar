#include "cursiv/bench/LanguageBench.hpp"
#include "algol60/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      algol60::Algol60SharedServices, algol60::register_lsp_services>(argc, argv);
}
