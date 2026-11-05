#include "cursiv/LanguageBench.hpp"
#include "qif/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      qif::QifSharedServices, qif::register_lsp_services>(argc, argv);
}
