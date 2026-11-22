#include "cursiv/LanguageBench.hpp"
#include "properties/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      properties::PropertiesSharedServices, properties::register_lsp_services>(argc, argv);
}
