#include "cursiv/LanguageBench.hpp"
#include "mumath/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      mumath::MumathSharedServices, mumath::register_lsp_services>(argc, argv);
}
