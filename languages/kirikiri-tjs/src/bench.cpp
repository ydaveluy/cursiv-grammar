#include "cursiv/LanguageBench.hpp"
#include "kirikiritjs/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      kirikiritjs::KirikiriTjsSharedServices, kirikiritjs::register_lsp_services>(argc, argv);
}
