#include "cursiv/LanguageBench.hpp"
#include "logo/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      logo::LogoSharedServices, logo::register_lsp_services>(argc, argv);
}
