#include "cursiv/LanguageBench.hpp"
#include "infosapient/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      infosapient::InfosapientSharedServices, infosapient::register_lsp_services>(argc, argv);
}
