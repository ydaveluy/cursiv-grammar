#include "cursiv/LanguageBench.hpp"
#include "acme/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      acme::AcmeSharedServices, acme::register_lsp_services>(argc, argv);
}
