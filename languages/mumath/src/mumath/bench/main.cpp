#include "cursiv/bench/LanguageBench.hpp"
#include "mumath/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      mumath::MumathSharedServices, mumath::register_lsp_services>(argc, argv);
}
