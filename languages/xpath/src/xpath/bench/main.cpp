#include "cursiv/bench/LanguageBench.hpp"
#include "xpath/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      xpath::XpathSharedServices, xpath::register_lsp_services>(argc, argv);
}
