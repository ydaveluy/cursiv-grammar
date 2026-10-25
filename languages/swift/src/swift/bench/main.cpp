#include "cursiv/bench/LanguageBench.hpp"
#include "swift/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      swift::SwiftSharedServices, swift::register_lsp_services>(argc, argv);
}
