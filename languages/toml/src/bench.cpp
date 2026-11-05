#include "cursiv/LanguageBench.hpp"
#include "toml/LspModule.hpp"

int main(int argc, char** argv) {
  return cursiv::bench::run_language<
      toml::TomlSharedServices, toml::register_lsp_services>(argc, argv);
}
