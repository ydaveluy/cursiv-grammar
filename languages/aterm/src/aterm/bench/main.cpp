#include "cursiv/bench/LanguageBench.hpp"
#include "aterm/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      aterm::AtermSharedServices, aterm::register_lsp_services>(argc, argv);
}
