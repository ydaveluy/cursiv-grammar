#include "cursiv/LanguageBench.hpp"
#include "muddb/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      muddb::MuddbSharedServices, muddb::register_lsp_services>(argc, argv);
}
