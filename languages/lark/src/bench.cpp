#include "cursiv/LanguageBench.hpp"
#include "lark/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      lark::LarkSharedServices, lark::register_lsp_services>(argc, argv);
}
