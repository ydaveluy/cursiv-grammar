#include "cursiv/bench/LanguageBench.hpp"
#include "logo/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      logo::LogoSharedServices, logo::register_lsp_services>(argc, argv);
}
