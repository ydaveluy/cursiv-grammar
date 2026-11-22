#include "cursiv/LanguageBench.hpp"
#include "dif/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      dif::DifSharedServices, dif::register_lsp_services>(argc, argv);
}
