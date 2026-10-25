#include "cursiv/bench/LanguageBench.hpp"
#include "xyz/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      xyz::XyzSharedServices, xyz::register_lsp_services>(argc, argv);
}
