#include "cursiv/bench/LanguageBench.hpp"
#include "hexpat/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      hexpat::HexpatSharedServices, hexpat::register_lsp_services>(argc, argv);
}
