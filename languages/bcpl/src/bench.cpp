#include "cursiv/LanguageBench.hpp"
#include "bcpl/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      bcpl::BcplSharedServices, bcpl::register_lsp_services>(argc, argv);
}
