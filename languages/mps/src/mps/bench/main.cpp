#include "cursiv/bench/LanguageBench.hpp"
#include "mps/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      mps::MpsSharedServices, mps::register_lsp_services>(argc, argv);
}
