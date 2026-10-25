#include "cursiv/bench/LanguageBench.hpp"
#include "tiny/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      tiny::TinySharedServices, tiny::register_lsp_services>(argc, argv);
}
