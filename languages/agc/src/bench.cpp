#include "cursiv/LanguageBench.hpp"
#include "agc/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      agc::AgcSharedServices, agc::register_lsp_services>(argc, argv);
}
