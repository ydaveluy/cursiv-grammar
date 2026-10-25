#include "cursiv/bench/LanguageBench.hpp"
#include "lrc/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      lrc::LrcSharedServices, lrc::register_lsp_services>(argc, argv);
}
