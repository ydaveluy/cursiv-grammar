#include "cursiv/LanguageBench.hpp"
#include "lolcode/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      lolcode::LolcodeSharedServices, lolcode::register_lsp_services>(argc, argv);
}
