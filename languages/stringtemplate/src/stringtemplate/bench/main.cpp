#include "cursiv/bench/LanguageBench.hpp"
#include "stringtemplate/lsp/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      stringtemplate::StringtemplateSharedServices, stringtemplate::register_lsp_services>(argc, argv);
}
