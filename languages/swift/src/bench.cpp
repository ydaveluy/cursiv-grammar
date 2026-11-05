#include "cursiv/LanguageBench.hpp"
#include "swift/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      swift::SwiftSharedServices, swift::register_lsp_services>(argc, argv);
}
