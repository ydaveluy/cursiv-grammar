#include "cursiv/bench/LanguageBench.hpp"
#include "dif/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      dif::DifSharedServices, dif::register_lsp_services>(argc, argv);
}
