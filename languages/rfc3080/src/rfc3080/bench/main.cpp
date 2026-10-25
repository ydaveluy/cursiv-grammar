#include "cursiv/bench/LanguageBench.hpp"
#include "rfc3080/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      rfc3080::Rfc3080SharedServices, rfc3080::register_lsp_services>(argc, argv);
}
