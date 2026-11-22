#include "cursiv/LanguageBench.hpp"
#include "powerbuilder/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      powerbuilder::PowerbuilderSharedServices, powerbuilder::register_lsp_services>(argc, argv);
}
