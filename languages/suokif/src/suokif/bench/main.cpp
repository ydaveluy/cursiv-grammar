#include "cursiv/bench/LanguageBench.hpp"
#include "suokif/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      suokif::SuokifSharedServices, suokif::register_lsp_services>(argc, argv);
}
