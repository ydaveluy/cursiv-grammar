#include "cursiv/LanguageBench.hpp"
#include "databank/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      databank::DatabankSharedServices, databank::register_lsp_services>(argc, argv);
}
