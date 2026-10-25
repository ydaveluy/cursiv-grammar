#include "cursiv/bench/LanguageBench.hpp"
#include "lark/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      lark::LarkSharedServices, lark::register_lsp_services>(argc, argv);
}
