#include "cursiv/bench/LanguageBench.hpp"
#include "wat/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      wat::WatSharedServices, wat::register_lsp_services>(argc, argv);
}
