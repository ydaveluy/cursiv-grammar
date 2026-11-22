#include "cursiv/LanguageBench.hpp"
#include "yini/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      yini::YiniSharedServices, yini::register_lsp_services>(argc, argv);
}
