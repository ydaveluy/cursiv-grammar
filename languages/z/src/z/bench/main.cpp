#include "cursiv/bench/LanguageBench.hpp"
#include "z/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      z::ZSharedServices, z::register_lsp_services>(argc, argv);
}
