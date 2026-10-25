#include "cursiv/bench/LanguageBench.hpp"
#include "bdf/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      bdf::BdfSharedServices, bdf::register_lsp_services>(argc, argv);
}
