#include "cursiv/bench/LanguageBench.hpp"
#include "infosapient/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      infosapient::InfosapientSharedServices, infosapient::register_lsp_services>(argc, argv);
}
