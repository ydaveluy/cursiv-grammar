#include "cursiv/bench/LanguageBench.hpp"
#include "fdo91/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      fdo91::Fdo91SharedServices, fdo91::register_lsp_services>(argc, argv);
}
