#include "cursiv/LanguageBench.hpp"
#include "xpath/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      xpath::XpathSharedServices, xpath::register_lsp_services>(argc, argv);
}
