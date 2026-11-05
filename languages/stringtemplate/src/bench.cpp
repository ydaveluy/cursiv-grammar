#include "cursiv/LanguageBench.hpp"
#include "stringtemplate/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      stringtemplate::StringtemplateSharedServices, stringtemplate::register_lsp_services>(argc, argv);
}
