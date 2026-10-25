#include "cursiv/bench/LanguageBench.hpp"
#include "tl/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      tl::TlSharedServices, tl::register_lsp_services>(argc, argv);
}
