#include "cursiv/LanguageBench.hpp"
#include "toml/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::runLanguage<
      toml::TomlSharedServices, toml::register_lsp_services>(argc, argv);
}
