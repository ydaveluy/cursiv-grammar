#include "cursiv/LanguageBench.hpp"
#include "rcs/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      rcs::RcsSharedServices, rcs::register_lsp_services>(argc, argv);
}
