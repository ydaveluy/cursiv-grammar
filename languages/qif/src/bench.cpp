#include "cursiv/LanguageBench.hpp"
#include "qif/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      qif::QifSharedServices, qif::register_lsp_services>(argc, argv);
}
