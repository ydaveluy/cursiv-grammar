#include "cursiv/bench/LanguageBench.hpp"
#include "qif/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      qif::QifSharedServices, qif::register_lsp_services>(argc, argv);
}
