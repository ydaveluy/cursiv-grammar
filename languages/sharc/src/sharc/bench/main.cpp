#include "cursiv/bench/LanguageBench.hpp"
#include "sharc/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      sharc::SharcSharedServices, sharc::register_lsp_services>(argc, argv);
}
