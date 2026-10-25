#include "cursiv/bench/LanguageBench.hpp"
#include "kirikiritjs/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      kirikiritjs::KirikiriTjsSharedServices, kirikiritjs::register_lsp_services>(argc, argv);
}
