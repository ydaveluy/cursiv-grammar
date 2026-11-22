#include "cursiv/LanguageBench.hpp"
#include "ruleworks/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      ruleworks::RuleworksSharedServices, ruleworks::register_lsp_services>(argc, argv);
}
