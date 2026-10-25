#include "cursiv/bench/LanguageBench.hpp"
#include "scotty/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      scotty::ScottySharedServices, scotty::register_lsp_services>(argc, argv);
}
