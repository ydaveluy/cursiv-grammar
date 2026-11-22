#include "cursiv/LanguageBench.hpp"
#include "ctl/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      ctl::CtlSharedServices, ctl::register_lsp_services>(argc, argv);
}
