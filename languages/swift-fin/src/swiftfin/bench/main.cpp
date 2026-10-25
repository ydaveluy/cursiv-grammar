#include "cursiv/bench/LanguageBench.hpp"
#include "swiftfin/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      swiftfin::SwiftFinSharedServices, swiftfin::register_lsp_services>(argc, argv);
}
