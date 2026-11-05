#include "cursiv/LanguageBench.hpp"
#include "spass/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      spass::SpassSharedServices, spass::register_lsp_services>(argc, argv);
}
