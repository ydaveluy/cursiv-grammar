#include "cursiv/bench/LanguageBench.hpp"
#include "gvpr/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      gvpr::GvprSharedServices, gvpr::register_lsp_services>(argc, argv);
}
