#include "cursiv/bench/LanguageBench.hpp"
#include "icon/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      icon::IconSharedServices, icon::register_lsp_services>(argc, argv);
}
