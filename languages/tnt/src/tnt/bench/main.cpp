#include "cursiv/bench/LanguageBench.hpp"
#include "tnt/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      tnt::TntSharedServices, tnt::register_lsp_services>(argc, argv);
}
