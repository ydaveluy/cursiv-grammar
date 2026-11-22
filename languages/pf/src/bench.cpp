#include "cursiv/LanguageBench.hpp"
#include "pf/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      pf::PfSharedServices, pf::register_lsp_services>(argc, argv);
}
