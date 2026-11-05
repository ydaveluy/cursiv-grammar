#include "cursiv/LanguageBench.hpp"
#include "hexpat/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      hexpat::HexpatSharedServices, hexpat::register_lsp_services>(argc, argv);
}
