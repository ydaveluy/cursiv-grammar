#include "cursiv/LanguageBench.hpp"
#include "ctl/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      ctl::CtlSharedServices, ctl::register_lsp_services>(argc, argv);
}
