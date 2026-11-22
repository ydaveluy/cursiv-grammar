#include "cursiv/LanguageBench.hpp"
#include "pii/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      pii::PiiSharedServices, pii::register_lsp_services>(argc, argv);
}
